// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_I18N_ICU_ENCODING_DETECTION_H_
#define BASE_I18N_ICU_ENCODING_DETECTION_H_
#pragma once

#include <string>
#include <vector>

namespace base {

// Detect encoding of |text| and put the name of encoding (as returned by ICU)
// in |encoding|. For ASCII texts |encoding| will be set to an empty string.
// Returns true on success.
bool DetectEncoding(const std::string& text, std::string* encoding);

// Detect all possible encodings of |text| and put their names
// (as returned by ICU) in |encodings|. Returns true on success.
bool DetectAllEncodings(const std::string& text,
                        std::vector<std::string>* encodings);

}  // namespace base

#endif  // BASE_I18N_ICU_ENCODING_DETECTION_H_
