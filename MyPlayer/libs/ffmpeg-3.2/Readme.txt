                                  ffmpeg FULL SDK V3.2
                                       ��2008-04-12��



--------------------------------------------------------------------------------

    ��ffmpeg�����飨http://www.ffmpeg.com.cn��2006��5��5�շ���ffmpeg+x264 SDK V1.0�汾��2006��9��25�շ���V2.0�汾֮��2007��8��21�շ���V3.0�汾���кܳ���ʱ��û�з������µ�SDK�汾��ffmpeg��SVN�汾��Ҳ��V3.0�汾ʱ��r10087���µ���Ŀǰ��r12790�����ڼ�ffmpeg�Ĺ������ȶ��Զ������˺ܶ࣬���ֱ�����������Ҳ���˽ϴ���ȵ��������ںܶ����ĵ�����ǿ��Ҫ���£�ffmpeg�������ٴη���FFMPEG FULL SDK V3.2���˴η�����SDK����ΪFull SDK�����Ǿ����ܵļ��ɸ���ı����������Ҫע����ǣ��°汾��SDK�е�ffmpeg.exe������֧�ֵ�ָ����V3.0��֧�ֵ�ָ�����˽ϴ�Ĳ�ͬ�������ʹ��ָ�ffmpeg.exe --help����ø���İ�����

    ��SDK�ı����������£�
./configure --prefix=d:/OmniCoder/svn_build --enable-memalign-hack --enable-shared --disable-static --disable-encod
er=snow --disable-decoder=ac3 --disable-decoder=vorbis --disable-encoder=vorbis --disable-vhook --enable-ffplay --disabl
e-ffserver --disable-mpegaudio-hp --enable-pthreads --enable-liba52 --enable-nonfree --enable-libamr-nb --enable-libamr-
wb --enable-libfaac --enable-libfaad --enable-libmp3lame --enable-libgsm --enable-libtheora --enable-libvorbis --enable-
libx264 --enable-libxvid --enable-avisynth --enable-gpl --enable-swscale --enable-avfilter --enable-avfilter-lavf --enab
le-demuxer=vfwcap --enable-demuxer=rm --extra-cflags=-I/usr/local/include -I/usr/local/include/SDL --extra-ldflags=-L/us
r/local/lib --extra-libs=-lpthreadGC2

  libavutil version: 49.6.0
  libavcodec version: 51.54.0
  libavformat version: 52.13.0
  libavdevice version: 52.0.0
  libavfilter version: 0.0.0
  built on Apr 12 2008 22:21:14, gcc: 4.3.0

Note:
output_example.exe, tools/cws2fws.exe, tools/pktdumper.exe, tools/qt-faststart.exe, tools/trasher.exe are also compiled

H264 decoding/encoding should be ehanced huge in this version if you have multi-core CPUs
x264 encoding will use multi-cores automically in this version

eg:
ffmpeg -i input.avi -threads 2 -vcodec libx264 -b 500K -bufsize 500K  -minrate 500K -maxrate 500K -rc_eq "blurCplx^(1-qComp)" -qmin 2 -qmax 51 output.mp4

Addins:
1) how to make a mp4 streamable with Adobe Flash Player?
You can use the following command to make a mp4/mov/3gp file<encoded with H264+AAC> streaming feature which is compatible with the latest Adobe Flash Player
eg:
 qt-faststart.exe input.mp4 output.mp4
 
Now,output.mp4 can be rendered instantly by Flash Player

2) VFW camera capture
The following command will use your default camera device to preview&grap picture,and encode the picture to mpeg4

ffmpeg -f vfwcap -s 320x240 -r 25 -i 0 -b 400K -vcodec mpeg4 new.mp4

    SDK��ʹ�÷����ɲο�ffmpeg��Ŀ�е�output_example.c��libavcodecĿ¼�µ�apiexample.c�Լ�ffmpeg.c������Դ���롣

    ���ǽ�����������ǿ�󡢸��ȶ���ffmpeg SDK���������עffmpeg�����飨http://www.ffmpeg.com.cn��

�ļ���
    ѹ�����ڰ�������Ŀ¼����include������lib���͡�bin������Ŀ¼�����С�include��Ŀ¼�����˸�SDK����Ҫ�õ�������ͷ�ļ���ͨ��������£�ֻ��Ҫ����Ĺ����м��롰#include "libavcodec/avcodec.h"����ͷ�ļ��İ������ɣ���lib��Ŀ¼���������ӹ���ʱ����Ҫ��.lib���ļ�����bin��Ŀ¼���������г�������Ҫ��.dll��̬���ļ���������ִ�г�����ʹ�ø�SDK���п���ʱ����Ҫ��.lib���ļ����빤���С�
    ѹ�����ڵġ�bin��Ŀ¼�л������ˡ�ffmpeg.exe����ִ���ļ�����ffmpeg.exe����ffmpeg�����е�һ����Ŀ����Ҫ������Ƶ�ļ���ת������ͼ�Ȳ�������һ��ǳ�ǿ���Ӧ�ó�����ffmpeg�Ĺ����п����ҵ�����Դ���롣

�汾��
    ffmpeg��V0.4.9 pre1 (SVN update:2008-04-12 SVN r12790)

Դ���룺
    ��SDK�����ṩffmpeg��ȫ��Դ���룬�����Ҫ������ͨ�����µ�ַ�ṩ�Ķ��ַ�ʽ��ȡ��
    http://www.ffmpeg.com.cn/index.php/%E8%8E%B7%E5%8F%96ffmpeg%E6%BA%90%E4%BB%A3%E7%A0%81


���ʣ�
    �����ʹ�ù����������������⣬����ͨ�����·�ʽ�õ���صļ���֧�֣�
    ffmpeg�����飺http://www.ffmpeg.com.cn
    ffmpeg�����飨��̳����http://bbs.chinavideo.org
    ffmpeg������QQȺ��6939161��27548839
    GoogleGroups��http://groups.google.com/group/smartAV


��л��
    ��SDK������ffmpeg�������ԱFastreaming���룬���κ������ҿ���ͨ������ķ�ʽ��֮��ϵ��
--
Skype: fastreaming
GTalk:  fastreaming
MSN:   dev@fastreaming.com
Mail:    codec@fastreaming.com
******We are here just for you******


                        �л���Ƶ����  http://www.chinavideo.org
                        ffmpeg�����飺http://www.ffmpeg.com.cn
                        ����Ƽ��� http://www.bairuitech.com
                                  2008��4��12��