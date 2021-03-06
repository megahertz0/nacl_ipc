// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Unit tests for event trace consumer_ base class.
#include "base/win/event_trace_consumer.h"
#include <list>
#include "base/basictypes.h"
#include "base/win/event_trace_controller.h"
#include "base/win/event_trace_provider.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/scoped_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

#include <initguid.h>  // NOLINT - has to be last

namespace {

using base::win::EtwMofEvent;
using base::win::EtwTraceController;
using base::win::EtwTraceConsumerBase;
using base::win::EtwTraceProperties;
using base::win::EtwTraceProvider;

typedef std::list<EVENT_TRACE> EventQueue;

class TestConsumer: public EtwTraceConsumerBase<TestConsumer> {
 public:
  TestConsumer() {
    sank_event_.Set(::CreateEvent(NULL, TRUE, FALSE, NULL));
    ClearQueue();
  }

  ~TestConsumer() {
    ClearQueue();
    sank_event_.Close();
  }

  void ClearQueue() {
    EventQueue::const_iterator it(events_.begin()), end(events_.end());

    for (; it != end; ++it) {
      delete [] it->MofData;
    }

    events_.clear();
  }

  static void EnqueueEvent(EVENT_TRACE* event) {
    events_.push_back(*event);
    EVENT_TRACE& back = events_.back();

    if (NULL != event->MofData && 0 != event->MofLength) {
      back.MofData = new char[event->MofLength];
      memcpy(back.MofData, event->MofData, event->MofLength);
    }
  }

  static void ProcessEvent(EVENT_TRACE* event) {
    EnqueueEvent(event);
    ::SetEvent(sank_event_.Get());
  }

  static ScopedHandle sank_event_;
  static EventQueue events_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestConsumer);
};

ScopedHandle TestConsumer::sank_event_;
EventQueue TestConsumer::events_;

const wchar_t* const kTestSessionName = L"TestLogSession";

class EtwTraceConsumerBaseTest: public testing::Test {
 public:
  virtual void SetUp() {
    EtwTraceProperties ignore;
    EtwTraceController::Stop(kTestSessionName, &ignore);
  }
};

}  // namespace

TEST_F(EtwTraceConsumerBaseTest, Initialize) {
  TestConsumer consumer_;
}

TEST_F(EtwTraceConsumerBaseTest, OpenRealtimeSucceedsWhenNoSession) {
  TestConsumer consumer_;

  ASSERT_HRESULT_SUCCEEDED(consumer_.OpenRealtimeSession(kTestSessionName));
}

TEST_F(EtwTraceConsumerBaseTest, ConsumerImmediateFailureWhenNoSession) {
  TestConsumer consumer_;

  ASSERT_HRESULT_SUCCEEDED(consumer_.OpenRealtimeSession(kTestSessionName));
  ASSERT_HRESULT_FAILED(consumer_.Consume());
}

namespace {

class EtwTraceConsumerRealtimeTest: public testing::Test {
 public:
  virtual void SetUp() {
    ASSERT_HRESULT_SUCCEEDED(consumer_.OpenRealtimeSession(kTestSessionName));
  }

  virtual void TearDown() {
    consumer_.Close();
  }

  DWORD ConsumerThread() {
    ::SetEvent(consumer_ready_.Get());

    HRESULT hr = consumer_.Consume();
    return hr;
  }

  static DWORD WINAPI ConsumerThreadMainProc(void* arg) {
    return reinterpret_cast<EtwTraceConsumerRealtimeTest*>(arg)->
        ConsumerThread();
  }

  HRESULT StartConsumerThread() {
    consumer_ready_.Set(::CreateEvent(NULL, TRUE, FALSE, NULL));
    EXPECT_TRUE(consumer_ready_ != NULL);
    consumer_thread_.Set(::CreateThread(NULL, 0, ConsumerThreadMainProc,
        this, 0, NULL));
    if (NULL == consumer_thread_.Get())
      return HRESULT_FROM_WIN32(::GetLastError());

    HRESULT hr = S_OK;
    HANDLE events[] = { consumer_ready_, consumer_thread_ };
    DWORD result = ::WaitForMultipleObjects(arraysize(events), events,
                                            FALSE, INFINITE);
    switch (result) {
      case WAIT_OBJECT_0:
        // The event was set, the consumer_ is ready.
        return S_OK;
      case WAIT_OBJECT_0 + 1: {
          // The thread finished. This may race with the event, so check
          // explicitly for the event here, before concluding there's trouble.
          if (WAIT_OBJECT_0 == ::WaitForSingleObject(consumer_ready_, 0))
            return S_OK;
          DWORD exit_code = 0;
          if (::GetExitCodeThread(consumer_thread_, &exit_code))
            return exit_code;
          else
            return HRESULT_FROM_WIN32(::GetLastError());
          break;
        }
      default:
        return E_UNEXPECTED;
        break;
    }

    return hr;
  }

  // Waits for consumer_ thread to exit, and returns its exit code.
  HRESULT JoinConsumerThread() {
    if (WAIT_OBJECT_0 != ::WaitForSingleObject(consumer_thread_, INFINITE))
      return HRESULT_FROM_WIN32(::GetLastError());

    DWORD exit_code = 0;
    if (::GetExitCodeThread(consumer_thread_, &exit_code))
      return exit_code;

    return HRESULT_FROM_WIN32(::GetLastError());
  }

  TestConsumer consumer_;
  ScopedHandle consumer_ready_;
  ScopedHandle consumer_thread_;
};
}  // namespace

TEST_F(EtwTraceConsumerRealtimeTest, ConsumerReturnsWhenSessionClosed) {
  EtwTraceController controller;

  HRESULT hr = controller.StartRealtimeSession(kTestSessionName, 100 * 1024);
  if (hr == E_ACCESSDENIED) {
    VLOG(1) << "You must be an administrator to run this test on Vista";
    return;
  }

  // Start the consumer_.
  ASSERT_HRESULT_SUCCEEDED(StartConsumerThread());

  // Wait around for the consumer_ thread a bit.
  ASSERT_EQ(WAIT_TIMEOUT, ::WaitForSingleObject(consumer_thread_, 50));

  ASSERT_HRESULT_SUCCEEDED(controller.Stop(NULL));

  // The consumer_ returns success on session stop.
  ASSERT_HRESULT_SUCCEEDED(JoinConsumerThread());
}

namespace {

// {036B8F65-8DF3-46e4-ABFC-6985C43D59BA}
DEFINE_GUID(kTestProvider,
  0x36b8f65, 0x8df3, 0x46e4, 0xab, 0xfc, 0x69, 0x85, 0xc4, 0x3d, 0x59, 0xba);

// {57E47923-A549-476f-86CA-503D57F59E62}
DEFINE_GUID(kTestEventType,
  0x57e47923, 0xa549, 0x476f, 0x86, 0xca, 0x50, 0x3d, 0x57, 0xf5, 0x9e, 0x62);

}  // namespace

TEST_F(EtwTraceConsumerRealtimeTest, ConsumeEvent) {
  EtwTraceController controller;
  HRESULT hr = controller.StartRealtimeSession(kTestSessionName, 100 * 1024);
  if (hr == E_ACCESSDENIED) {
    VLOG(1) << "You must be an administrator to run this test on Vista";
    return;
  }

  ASSERT_HRESULT_SUCCEEDED(controller.EnableProvider(kTestProvider,
      TRACE_LEVEL_VERBOSE, 0xFFFFFFFF));

  EtwTraceProvider provider(kTestProvider);
  ASSERT_EQ(ERROR_SUCCESS, provider.Register());

  // Start the consumer_.
  ASSERT_HRESULT_SUCCEEDED(StartConsumerThread());

  ASSERT_EQ(0, TestConsumer::events_.size());

  EtwMofEvent<1> event(kTestEventType, 1, TRACE_LEVEL_ERROR);
  EXPECT_EQ(ERROR_SUCCESS, provider.Log(&event.header));

  EXPECT_EQ(WAIT_OBJECT_0, ::WaitForSingleObject(TestConsumer::sank_event_,
                                                 INFINITE));
  ASSERT_HRESULT_SUCCEEDED(controller.Stop(NULL));
  ASSERT_HRESULT_SUCCEEDED(JoinConsumerThread());
  ASSERT_NE(0u, TestConsumer::events_.size());
}

namespace {

// We run events through a file session to assert that
// the content comes through.
class EtwTraceConsumerDataTest: public testing::Test {
 public:
  EtwTraceConsumerDataTest() {
  }

  virtual void SetUp() {
    EtwTraceProperties prop;
    EtwTraceController::Stop(kTestSessionName, &prop);
    // Construct a temp file name.
    ASSERT_TRUE(file_util::CreateTemporaryFile(&temp_file_));
  }

  virtual void TearDown() {
    EXPECT_TRUE(file_util::Delete(temp_file_, false));
    EtwTraceProperties ignore;
    EtwTraceController::Stop(kTestSessionName, &ignore);
  }

  HRESULT LogEventToTempSession(PEVENT_TRACE_HEADER header) {
    EtwTraceController controller;

    // Set up a file session.
    HRESULT hr = controller.StartFileSession(kTestSessionName,
                                             temp_file_.value().c_str());
    if (FAILED(hr))
      return hr;

    // Enable our provider.
    EXPECT_HRESULT_SUCCEEDED(controller.EnableProvider(kTestProvider,
        TRACE_LEVEL_VERBOSE, 0xFFFFFFFF));

    EtwTraceProvider provider(kTestProvider);
    // Then register our provider, means we get a session handle immediately.
    EXPECT_EQ(ERROR_SUCCESS, provider.Register());
    // Trace the event, it goes to the temp file.
    EXPECT_EQ(ERROR_SUCCESS, provider.Log(header));
    EXPECT_HRESULT_SUCCEEDED(controller.DisableProvider(kTestProvider));
    EXPECT_HRESULT_SUCCEEDED(provider.Unregister());
    EXPECT_HRESULT_SUCCEEDED(controller.Flush(NULL));
    EXPECT_HRESULT_SUCCEEDED(controller.Stop(NULL));

    return S_OK;
  }

  HRESULT ConsumeEventFromTempSession() {
    // Now consume the event(s).
    TestConsumer consumer_;
    HRESULT hr = consumer_.OpenFileSession(temp_file_.value().c_str());
    if (SUCCEEDED(hr))
      hr = consumer_.Consume();
    consumer_.Close();
    // And nab the result.
    events_.swap(TestConsumer::events_);
    return hr;
  }

  HRESULT RoundTripEvent(PEVENT_TRACE_HEADER header, PEVENT_TRACE* trace) {
    file_util::Delete(temp_file_, false);

    HRESULT hr = LogEventToTempSession(header);
    if (SUCCEEDED(hr))
      hr = ConsumeEventFromTempSession();

    if (FAILED(hr))
      return hr;

    // We should now have the event in the queue.
    if (events_.empty())
      return E_FAIL;

    *trace = &events_.back();
    return S_OK;
  }

  EventQueue events_;
  FilePath temp_file_;
};

}  // namespace


TEST_F(EtwTraceConsumerDataTest, RoundTrip) {
  EtwMofEvent<1> event(kTestEventType, 1, TRACE_LEVEL_ERROR);

  static const char kData[] = "This is but test data";
  event.fields[0].DataPtr = reinterpret_cast<ULONG64>(kData);
  event.fields[0].Length = sizeof(kData);

  PEVENT_TRACE trace = NULL;
  HRESULT hr = RoundTripEvent(&event.header, &trace);
  if (hr == E_ACCESSDENIED) {
    VLOG(1) << "You must be an administrator to run this test on Vista";
    return;
  }
  ASSERT_TRUE(NULL != trace);
  ASSERT_EQ(sizeof(kData), trace->MofLength);
  ASSERT_STREQ(kData, reinterpret_cast<const char*>(trace->MofData));
}
