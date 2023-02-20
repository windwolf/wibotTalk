//
// Created by zhouj on 2023/2/20.
//

#ifndef AQ_DJ_2022_LIBS_WIBOTTALK_BASIC_CHECKSUMVALIDATOR_HPP_
#define AQ_DJ_2022_LIBS_WIBOTTALK_BASIC_CHECKSUMVALIDATOR_HPP_
#include "DataValidatorBase.hpp"

namespace wibot
{

    class CheckSum8Validator : public DataValidatorBase
    {
     public:
        void reset() override;
        void calulate(uint8_t* data, uint32_t length) override;
        bool validate(uint8_t* sum, uint32_t length) override;
     private:
        uint8_t sum_;
    };

} // wibot

#endif //AQ_DJ_2022_LIBS_WIBOTTALK_BASIC_CHECKSUMVALIDATOR_HPP_
