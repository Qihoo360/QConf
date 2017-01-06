#ifndef CONSTDEF_H
#define CONSTDEF_H

#include <string>

//return status
enum RetCode {
  kSuccess = 0,
  kOtherError,
  kNotExist,
  kOpenFileFailed,
  kParamError,
  kZkFailed
};


// server status define
enum kServerStatus {
  kStatusUnknow = -1,
  kStatusUp,
  kStatusOffline,
  kStatusDown
};

#endif  // CONSTDEF_H
