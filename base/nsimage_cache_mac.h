// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_NSIMAGE_CACHE_MAC_H_
#define BASE_NSIMAGE_CACHE_MAC_H_
#pragma once

#ifdef __OBJC__
@class NSImage;
@class NSString;
#else
class NSImage;
class NSString;
#endif

namespace nsimage_cache {

// Returns an autoreleased image from the main app bundle
// (mac_util::MainAppBundle()) with the given name, and keeps it in memory so
// future fetches are fast.
// NOTE:
//   - This should only be called on the main thread.
//   - The caller should retain the image if they want to keep it around, as
//     the cache could have limit on size/lifetime, etc.
NSImage* ImageNamed(NSString* name);

// Clears the cache.
void Clear(void);

}  // namespace nsimage_cache

#endif  // BASE_NSIMAGE_CACHE_MAC_H_
