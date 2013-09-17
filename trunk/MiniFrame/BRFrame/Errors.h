#ifndef __ERRORS_H__
#define __ERRORS_H__

#include "Common.h"

namespace interface_error {
    enum errors {
        already_started,
    };
    namespace detail {
        class interface_error_category 
            : public boost::system::error_category {
        public:
            const char * name() const 
            {
                return "interface module";
            }

            std::string message(int value) const
            {
                switch (value)
                {
                case already_started:
                    return "already started!";
                default:
                    return "other error!";
                }
            }

        };
    } // namespace detail

    inline const boost::system::error_category & get_category()
    {
        static detail::interface_error_category instance;
        return instance;
    }
    inline boost::system::error_code make_error_code(
        errors e)
    {
        return boost::system::error_code(
            static_cast<int>(e), get_category());
    }
} // namespace interface_error

namespace boost {
    namespace system {
        template <>
        struct is_error_code_enum<interface_error::errors>
        {
            BOOST_STATIC_CONSTANT(bool, value = true);
        };
#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
        using interface_error::error::make_error_code;
#endif
    } // namespace system
} // namespace boost

#endif // __ERRORS_H__
