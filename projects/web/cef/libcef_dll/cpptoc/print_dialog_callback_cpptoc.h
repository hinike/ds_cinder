// Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool. If making changes by
// hand only do so within the body of existing method and function
// implementations. See the translator.README.txt file in the tools directory
// for more information.
//
// $hash=2226f122addf13cffaabae85c20db9b49f437129$
//

#ifndef CEF_LIBCEF_DLL_CPPTOC_PRINT_DIALOG_CALLBACK_CPPTOC_H_
#define CEF_LIBCEF_DLL_CPPTOC_PRINT_DIALOG_CALLBACK_CPPTOC_H_
#pragma once

#if !defined(BUILDING_CEF_SHARED)
#error This file can be included DLL-side only
#endif

#include "include/capi/cef_print_handler_capi.h"
#include "include/cef_print_handler.h"
#include "libcef_dll/cpptoc/cpptoc_ref_counted.h"

// Wrap a C++ class with a C structure.
// This class may be instantiated and accessed DLL-side only.
class CefPrintDialogCallbackCppToC
    : public CefCppToCRefCounted<CefPrintDialogCallbackCppToC,
                                 CefPrintDialogCallback,
                                 cef_print_dialog_callback_t> {
 public:
  CefPrintDialogCallbackCppToC();
};

#endif  // CEF_LIBCEF_DLL_CPPTOC_PRINT_DIALOG_CALLBACK_CPPTOC_H_
