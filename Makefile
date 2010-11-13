# Copyright 2010, The Native Client SDK Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can
# be found in the LICENSE file.

# Makefile for the Hello World example.

.PHONY: all clean

CCFILES = hello_world.cc \
          base/atomicops_internals_x86_gcc.cc \
          base/atomicops_unittest.cc \
          base/at_exit.cc \
          base/at_exit_unittest.cc \
          base/base_switches.cc \
          base/command_line.cc \
          base/command_line_unittest.cc \
          base/condition_variable_posix.cc \
          base/debug/debugger.cc \
          base/debug/debugger_posix.cc \
          base/debug/stack_trace.cc \
          base/debug/stack_trace_nacl.cc \
          base/debug/stack_trace_unittest.cc \
          base/file_path.cc \
          base/file_path_unittest.cc \
          base/json/json_reader.cc \
          base/json/json_reader_unittest.cc \
          base/json/json_writer.cc \
          base/json/json_writer_unittest.cc \
          base/json/string_escape.cc \
          base/json/string_escape_unittest.cc \
          base/lazy_instance.cc \
          base/lock.cc \
          base/lock_impl_posix.cc \
          base/logging.cc \
          base/message_loop_nacl.cc \
          base/pickle.cc \
          base/pickle_unittest.cc \
          base/platform_thread_posix.cc \
          base/platform_thread_unittest.cc \
          base/ref_counted.cc \
          base/ref_counted_unittest.cc \
          base/ref_counted_memory.cc \
          base/safe_strerror_posix.cc \
          base/string16.cc \
          base/string16_unittest.cc \
          base/stringprintf.cc \
          base/string_number_conversions.cc \
          base/string_number_conversions_unittest.cc \
          base/string_piece.cc \
          base/string_piece_unittest.cc \
          base/string_split.cc \
          base/string_util.cc \
          base/sys_string_conversions_linux.cc \
          base/sys_string_conversions_unittest.cc \
          base/task.cc \
          base/task_queue.cc \
          base/third_party/dmg_fp/dtoa.cc \
          base/third_party/dmg_fp/g_fmt.cc \
          base/third_party/icu/icu_utf.cc \
          base/third_party/nspr/prtime.cc \
          base/thread_collision_warner.cc \
          base/thread_local_posix.cc \
          base/thread_local_storage_posix.cc \
          base/time.cc \
          base/time_posix.cc \
          base/tracked.cc \
          base/tracked_objects.cc \
          base/tracked_objects_unittest.cc \
          base/utf_string_conversions.cc \
          base/utf_string_conversions_unittest.cc \
          base/utf_string_conversion_utils.cc \
          base/values.cc \
          base/vlog.cc \
          base/waitable_event_posix.cc \
          base/waitable_event_watcher_posix.cc \
          ipc/file_descriptor_set_posix.cc \
          ipc/file_descriptor_set_posix_unittest.cc \
          ipc/ipc_channel_nacl.cc \
          ipc/ipc_channel_proxy.cc \
          ipc/ipc_logging.cc \
          ipc/ipc_message.cc \
          ipc/ipc_message_unittest.cc \
          ipc/ipc_message_utils.cc \
          ipc/ipc_switches.cc \
          ipc/ipc_sync_channel.cc \
          ipc/ipc_sync_message.cc \
          ipc/ipc_sync_message_filter.cc \
          testing/gtest/src/gtest-death-test.cc \
          testing/gtest/src/gtest-filepath.cc \
          testing/gtest/src/gtest-port.cc \
          testing/gtest/src/gtest-printers.cc \
          testing/gtest/src/gtest-test-part.cc \
          testing/gtest/src/gtest-typed-test.cc \
          testing/gtest/src/gtest.cc \
          base/thread.cc \
          base/metrics/histogram.cc \
          base/message_loop_proxy.cc \
          base/message_loop_proxy_impl.cc \

          # base/condition_variable_unittest.cc HANGS
          # base/lazy_instance_unittest.cc LINK_ERROR
          # base/lock_unittest.cc HANGS
          # base/logging_unittest.cc COMPILE_ERROR
          # base/thread_local_unittest.cc LINK_ERROR
          # base/thread_local_storage_unittest.cc LINK_ERROR
          # base/time_unittest.cc CRASH

# Hangs, sadly.
#          ipc/ipc_sync_channel_unittest.cc \

# Can't seem to link w/o ipc_sync_channel_unittest
#          ipc/ipc_sync_message_unittest.cc \

# Depends on MultiProcessTest (which makes no sense for NaCl)
#          ipc/ipc_tests.cc \
#          ipc/ipc_fuzzing_tests.cc \

# Depends on EnableTerminationOnHeapDestruction (in process_util)
#          base/test/test_suite.cc \

# Requires gmock.h
#          base/string_split_unittest.cc \
#          base/string_util_unittest.cc \




OBJECTS_X86_32 = $(CCFILES:%.cc=%_x86_32.o)
OBJECTS_X86_64 = $(CCFILES:%.cc=%_x86_64.o)


# We could import the sdk into the git repo if we really wanted.
# for now, we assume that this is inside the examples folder or
# that they passed NACL_SDK_ROOT to make.
NACL_SDK_ROOT ?= ../..

CFLAGS = -Wall -Wno-long-long -pthread -DXP_UNIX -Werror -DUNIT_TEST
TESTING_INCLUDES = -I$(CURDIR)/testing/gtest \
                   -I$(CURDIR)/testing/gtest/include
INCLUDES = -I$(CURDIR) \
           -I$(NACL_SDK_ROOT) \
           $(TESTING_INCLUDES)
LDFLAGS = -lgoogle_nacl_imc \
          -lgoogle_nacl_npruntime \
          -lpthread \
          -lsrpc \
          -lnosys \
          $(ARCH_FLAGS)
OPT_FLAGS = -O2

all: check_variables hello_world_x86_32.nexe hello_world_x86_64.nexe

# common.mk has rules to build .o files from .cc files.
# common.mk comes from native_client_sdk_0_1_507_1/examples/common.mk
include common.mk

hello_world_x86_32.nexe: $(OBJECTS_X86_32)
	$(CPP) $^ $(LDFLAGS) -m32 -o $@

hello_world_x86_64.nexe: $(OBJECTS_X86_64)
	$(CPP) $^ $(LDFLAGS) -m64 -o $@

run:
	$(LDR) -- hello_world_x86_32.nexe

clean:
	-$(RM) $(OBJECTS_X86_32) $(OBJECTS_X86_64) \
	    hello_world_x86_32.nexe hello_world_x86_64.nexe

# This target is used by the SDK build system to produce a pre-built version
# of the .nexe.  You do not need to call this target.
install_prebuilt: hello_world_x86_32.nexe hello_world_x86_64.nexe
	-$(RM) $(OBJECTS_X86_32) $(OBJECTS_X86_64)
