//
// Created by zhouj on 2023/2/20.
//

#ifndef AQ_DJ_2022_LIBS_WIBOTTALK_BASIC_CRCBASE_HPP_
#define AQ_DJ_2022_LIBS_WIBOTTALK_BASIC_CRCBASE_HPP_

#include "base.hpp"

namespace wibot
{

    class DataValidatorBase
    {
        public:
        virtual void reset() = 0;
        virtual void calulate(uint8_t* data, uint32_t length) = 0;
        virtual bool validate(uint8_t* sum, uint32_t length) = 0;
    };

} // wibot

#endif //AQ_DJ_2022_LIBS_WIBOTTALK_BASIC_CRCBASE_HPP_
