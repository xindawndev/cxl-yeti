#include "YDCommon.h"
#include "YDTimer.h"
#include "YDEngine.h"

static yeti::dlna::ThreadPool * g_thread_pool = new yeti::dlna::ThreadPool();
static yeti::dlna::Engine * g_engine = new yeti::dlna::Engine();

int main(int argc, char ** argv)
{
    yeti::dlna::Timer tm(yeti::dlna::Engine::get_singleton());
    yeti::dlna::Engine::get_singleton().start(3);
    return 0;
}
