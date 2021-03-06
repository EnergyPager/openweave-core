/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This is a unit test suite for
 *      <tt>nl::Weave::System::Timer</tt>, * the part of the Weave
 *      System Layer that implements timers.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <SystemLayer/SystemConfig.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/tcpip.h>
#include <lwip/sys.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#include <sys/select.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#include <SystemLayer/SystemError.h>
#include <SystemLayer/SystemLayer.h>
#include <SystemLayer/SystemTimer.h>

#include <Weave/Support/ErrorStr.h>

#include <nltest.h>

using nl::ErrorStr;
using namespace nl::Weave::System;

static void ServiceEvents(Layer& aLayer, ::timeval& aSleepTime)
{
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    fd_set readFDs, writeFDs, exceptFDs;
    int numFDs = 0;

    FD_ZERO(&readFDs);
    FD_ZERO(&writeFDs);
    FD_ZERO(&exceptFDs);

    if (aLayer.State() == kLayerState_Initialized)
        aLayer.PrepareSelect(numFDs, &readFDs, &writeFDs, &exceptFDs, aSleepTime);

    int selectRes = select(numFDs, &readFDs, &writeFDs, &exceptFDs, &aSleepTime);
    if (selectRes < 0)
    {
        printf("select failed: %s\n", ErrorStr(MapErrorPOSIX(errno)));
        return;
    }
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (aLayer.State() == kLayerState_Initialized)
    {
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
        aLayer.HandleSelectResult(selectRes, &readFDs, &writeFDs, &exceptFDs);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        if (aLayer.State() == kLayerState_Initialized)
        {
            // TODO: Currently timers are delayed by aSleepTime above. A improved solution would have a mechanism to reduce
            // aSleepTime according to the next timer.
            aLayer.HandlePlatformTimer();
        }
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
    }
}

// Test input vector format.


struct TestContext {
    Layer* mLayer;
    nlTestSuite* mTestSuite;
};

// Test input data.


static struct TestContext sContext;

static volatile bool sOverflowTestDone;

void HandleTimer0Failed(Layer* inetLayer, void* aState, Error aError)
{
    TestContext& lContext = *static_cast<TestContext*>(aState);
    NL_TEST_ASSERT(lContext.mTestSuite, false);
    sOverflowTestDone = true;
}

void HandleTimer1Failed(Layer* inetLayer, void* aState, Error aError)
{
    TestContext& lContext = *static_cast<TestContext*>(aState);
    NL_TEST_ASSERT(lContext.mTestSuite, false);
    sOverflowTestDone = true;
}

void HandleTimer10Success(Layer* inetLayer, void* aState, Error aError)
{
    TestContext& lContext = *static_cast<TestContext*>(aState);
    NL_TEST_ASSERT(lContext.mTestSuite, true);
    sOverflowTestDone = true;
}

static void CheckOverflow(nlTestSuite* inSuite, void* aContext)
{
    uint32_t timeout_overflow_0ms = 652835029;
    uint32_t timeout_overflow_1ms = 1958505088;
    uint32_t timeout_10ms = 10;

    TestContext& lContext = *static_cast<TestContext*>(aContext);
    Layer& lSys = *lContext.mLayer;

    sOverflowTestDone = false;

    lSys.StartTimer(timeout_overflow_0ms, HandleTimer0Failed, aContext);
    lSys.StartTimer(timeout_overflow_1ms, HandleTimer1Failed, aContext);
    lSys.StartTimer(timeout_10ms, HandleTimer10Success, aContext);

    while (!sOverflowTestDone)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 1000; // 1 ms tick
        ServiceEvents(lSys, sleepTime);
    }

    lSys.CancelTimer(HandleTimer0Failed, aContext);
    lSys.CancelTimer(HandleTimer1Failed, aContext);
    lSys.CancelTimer(HandleTimer10Success, aContext);
}


// Test Suite


/**
 *   Test Suite. It lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Timer::TestOverflow",             CheckOverflow),
    NL_TEST_SENTINEL()
};

static int TestSetup(void* aContext);
static int TestTeardown(void* aContext);

static nlTestSuite kTheSuite = {
    "weave-system-timer",
    &sTests[0],
    TestSetup,
    TestTeardown
};

/**
 *  Set up the test suite.
 */
static int TestSetup(void* aContext)
{
    static Layer sLayer;

    TestContext& lContext = *reinterpret_cast<TestContext*>(aContext);
    void* lLayerContext = NULL;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    static sys_mbox* sLwIPEventQueue = NULL;

    sys_mbox_new(&sLwIPEventQueue, 100);
    tcpip_init(NULL, NULL);
    lLayerContext = &sLwIPEventQueue;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    sLayer.Init(lLayerContext);

    lContext.mLayer = &sLayer;
    lContext.mTestSuite = &kTheSuite;

    return (SUCCESS);
}

/**
 *  Tear down the test suite.
 *  Free memory reserved at TestSetup.
 */
static int TestTeardown(void* aContext)
{
    TestContext& lContext = *reinterpret_cast<TestContext*>(aContext);

    lContext.mLayer->Shutdown();

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_finish(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    return (SUCCESS);
}

int main(int argc, char *argv[])
{
    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit againt one lContext.
    nlTestRunner(&kTheSuite, &sContext);

    return nlTestRunnerStats(&kTheSuite);
}
