#include "YDCommon.h"
#include "YDEngine.h"

int main(int argc, char ** argv)
{
    yeti::dlna::Engine::get_singleton().start(3);
    return 0;
}
