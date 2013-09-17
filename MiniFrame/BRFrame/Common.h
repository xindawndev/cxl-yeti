#ifndef __COMMON_H__
#define __COMMON_H__

#include <boost/system/error_code.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <queue>
#include <iostream>

typedef boost::function<
void(boost::system::error_code const &, int)
> async_callback_func_type;
static async_callback_func_type placeholder_func;

typedef boost::function<
void(boost::system::error_code const &, int, async_callback_func_type)
> service_callback_func_type;

#endif // __COMMON_H__
