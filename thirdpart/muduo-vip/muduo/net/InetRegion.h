// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_INETREGION_H
#define MUDUO_NET_INETREGION_H

#include "muduo/base/copyable.h"
#include "muduo/base/StringPiece.h"

#include <netinet/in.h>

namespace muduo
{
namespace net
{

class InetRegion : public muduo::copyable
{
 public:
	 InetRegion() {}
  std::string country, location;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_INETREGION_H
