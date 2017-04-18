#include <stdlib.h>
#include <string.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
//SDL  
#include <SDL/SDL.h>
};

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio  

//Output PCM  
#define OUTPUT_PCM 0
//Use SDL  
#define USE_SDL 1

//Buffer:  
//|-----------|-------------|  
//chunk-------pos---len-----|  
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;

/* The audio function callback takes the following parameters:  
 * stream: A pointer to the audio buffer to be filled  
 * len: The length (in bytes) of the audio buffer  
 * 回调函数 
*/
void fill_audio(void *udata, Uint8 *stream, int len) {
    if(audio_len==0)        /*  Only  play  if  we  have  data  left  */
        return;

    len = len > audio_len ? audio_len : len;  /*  Mix  as  much  data  as  possible  */
    SDL_MixAudio(stream,audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
}

int main(int argc, char* argv[])
{
//    const char *url = "../test.aac";
    const char *url = "../test.mp3";
    //const char *url = "../test.wma";

    av_register_all();
    avformat_network_init();

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //Open  
    if(avformat_open_input(&pFormatCtx, url, NULL, NULL) !=0 ) {
        printf("Couldn't open input stream.\n");
        return -1;
    }
    // Retrieve stream information  
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    // Dump valid information onto standard error  
    av_dump_format(pFormatCtx, 0, url, false);

    // Find the first audio stream  
    int audioStream = -1;
    for(int i = 0; i < pFormatCtx->nb_streams; ++i)
        if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
            break;
        }

    if(audioStream == -1){
        printf("Didn't find a audio stream.\n");
        return -1;
    }

    // avcodec环境
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
    if (pCodecCtx == NULL)
    {
        printf("Could not allocate AVCodecContext\n");
        return -1;
    }
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[audioStream]->codecpar);

    // 获取avcodec[解码器]
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        printf("Codec not found.\n");
        return -1;
    }
    // 打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        printf("Could not open codec.\n");
        return -1;
    }


    FILE *pFile = NULL;
#if OUTPUT_PCM
    pFile=fopen("output.pcm", "wb");
#endif

    AVPacket *packet=(AVPacket *)malloc(sizeof(AVPacket));
    av_init_packet(packet);

    //Out Audio Param  
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    //AAC:1024  MP3:1152  
    int out_nb_samples = pCodecCtx->frame_size;
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_sample_rate = 44100;
    int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
    //Out Buffer Size  
    int out_buffer_size = av_samples_get_buffer_size(NULL,out_channels, out_nb_samples, out_sample_fmt, 1);

    uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);

    AVFrame *pFrame = av_frame_alloc();
//SDL------------------  
#if USE_SDL
    //Init  
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    //SDL_AudioSpec  
    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = out_sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = out_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = out_nb_samples;
    wanted_spec.callback = fill_audio;
    wanted_spec.userdata = pCodecCtx;

    if (SDL_OpenAudio(&wanted_spec, NULL) < 0){
        printf("can't open audio.\n");
        return -1;
    }
#endif
    printf("Bitrate:\t %3ld\n", pFormatCtx->bit_rate);
    printf("Decoder Name:\t %s\n", pCodecCtx->codec->long_name);
    printf("Channels:\t %d\n", pCodecCtx->channels);
    printf("Sample per Second\t %d \n", pCodecCtx->sample_rate);

    //FIX:Some Codec's Context Information is missing
    int64_t in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);
    //Swr  
    struct SwrContext *au_convert_ctx;
    au_convert_ctx = swr_alloc();
    au_convert_ctx = swr_alloc_set_opts(au_convert_ctx,
                                        out_channel_layout,
                                        out_sample_fmt,
                                        out_sample_rate,
                                        in_channel_layout,
                                        pCodecCtx->sample_fmt ,
                                        pCodecCtx->sample_rate,
                                        0,
                                        NULL);
    swr_init(au_convert_ctx);

    //Play  
    SDL_PauseAudio(0);

    int index = 0;
    while( av_read_frame(pFormatCtx, packet) >= 0 ) {
        if(packet->stream_index == audioStream) {
            if(0 != avcodec_send_packet(pCodecCtx, packet))
            {
                printf("Error in decoding audio frame.\n");
                break;

            }
            if(0 != avcodec_receive_frame(pCodecCtx, pFrame))
            {
                printf("Error in decoding audio frame.\n");
                break;
            }

            swr_convert(au_convert_ctx,
                        &out_buffer,
                        MAX_AUDIO_FRAME_SIZE,
                        (const uint8_t **)pFrame->data ,
                        pFrame->nb_samples);
            printf("index:%5d\t pts:%ld\t packet size:%d wanted.samples:%d current.samples:%d  \n", index, packet->pts, packet->size, wanted_spec.samples, pFrame->nb_samples);
            //FIX:FLAC,MP3,AAC Different number of samples
//            if(wanted_spec.samples != pFrame->nb_samples){
//                SDL_CloseAudio();
//                out_nb_samples = pFrame->nb_samples;
//                out_buffer_size = av_samples_get_buffer_size(NULL,
//                                                             out_channels,
//                                                             out_nb_samples,
//                                                             out_sample_fmt,
//                                                             1);
//                wanted_spec.samples = out_nb_samples;
//                SDL_OpenAudio(&wanted_spec, NULL);
//            }

#if OUTPUT_PCM
            //Write PCM
            fwrite(out_buffer, 1, out_buffer_size, pFile);
#endif
            index++;
//SDL------------------  
#if USE_SDL
            //Set audio buffer (PCM data)  
            audio_chunk = (Uint8 *) out_buffer; 
            //Audio buffer length  
            audio_len = out_buffer_size;

            audio_pos = audio_chunk;

            //Play
//            SDL_PauseAudio(0);
            while(audio_len > 0) // Wait until finish
                SDL_Delay(1);
#endif
        }
        av_packet_unref(packet);
    }
    swr_free(&au_convert_ctx);

#if USE_SDL
    SDL_CloseAudio();//Close SDL  
    SDL_Quit();
#endif

#if OUTPUT_PCM
    fclose(pFile);
#endif
    av_free(out_buffer);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}  