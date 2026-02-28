
#include <miniaudio.h>



class SoundMgr{

public:
    SoundMgr(){

    }

    ~SoundMgr(){
        std::cout << "[SoundMgr] Closing sound engine..." << std::endl;
        ma_engine_uninit(&engine_);
    }

    bool init(){
        std::cout << "[SoundMgr] Initializating sound engine..." << std::endl;
        ma_result res = ma_engine_init(NULL, &engine_);
        return (res == MA_SUCCESS) ? true : false;
    }

    void test(){
        ma_engine_play_sound(&engine_, "DefaultDance.mp3", NULL);
    }

    void test2(){
        ma_engine_play_sound(&engine_, "chinos.mp3", NULL);
    }



private:

    /************ Variables ********************************************************/

    ma_engine engine_;

};