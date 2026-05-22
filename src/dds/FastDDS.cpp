#include "dds/FastDDS.hpp"

#ifdef USE_FASTDDS

    // General ------------------------------------------------------------------------------

    FastDDS::FastDDS() {

    }

    FastDDS::~FastDDS() {

    }

#else

    // General ------------------------------------------------------------------------------
    FastDDS::FastDDS()  {}
    FastDDS::~FastDDS() {}

#endif
