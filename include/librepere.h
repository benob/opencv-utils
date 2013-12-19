#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdarg.h>
#include <zlib.h>
#include <string.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

struct RepereExtractKeyframe {
    typedef struct {
        char frame;
        int64_t byte;
        double time;
        int previous_frame;
    } frame_index;

    typedef struct {
        char* filename;
        int reopen;
        frame_index *local_index;
        int local_index_size, local_index_allocated;
        AVFormatContext *formatctx; 
        AVStream *vidst;
        AVCodecContext *codecctx;
        AVCodec *codec;
        AVPicture image;
        struct SwsContext *img_convert_ctx;
        AVFrame *vframe_;
        int vstream;
        int w_, h_;
    } repere_video;

    static int av_err(int err, const char *format, ...)
    {
        char buf[4096];
        va_list ap;
        if(!err)
            return 0;

        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);

        buf[0] = 0;
        av_strerror(err, buf, sizeof(buf));
        fprintf(stderr, ": %s\n",buf);
        return 1;
    }

    static void close_mpeg(repere_video* ctx) {
        avpicture_free(&ctx->image);
        avcodec_free_frame(&ctx->vframe_);
        avcodec_close(ctx->codecctx);
        avformat_close_input(&ctx->formatctx);
        sws_freeContext(ctx->img_convert_ctx);
        //memset(ctx, 0, sizeof(ctx));
    }

    static int open_mpeg(repere_video* ctx, const char *fname)
    {
        //memset(ctx, 0, sizeof(ctx));
        ctx->formatctx = NULL;
        //fprintf(stderr, "opening %s\n", fname);
        int err = avformat_open_input(&(ctx->formatctx), fname, NULL, NULL);
        if(av_err(err, "Failed while trying to open %s", fname)) return 0;

        err = avformat_find_stream_info(ctx->formatctx, NULL);
        if(av_err(err, "av_stream_info")) return 0;

        ctx->vstream = 0;
        while((ctx->vstream < (int)ctx->formatctx->nb_streams)
                && (ctx->formatctx->streams[ctx->vstream]->codec->codec_type != AVMEDIA_TYPE_VIDEO))
            ctx->vstream++;
        if(ctx->vstream > (int)ctx->formatctx->nb_streams) {
            fprintf(stderr, "No video substream\n");
            return 0;
        }

        ctx->vidst = ctx->formatctx->streams[ctx->vstream];
        ctx->codecctx = ctx->vidst->codec;
        ctx->codec = avcodec_find_decoder(ctx->codecctx->codec_id);
        if(!ctx->codec) {
            fprintf(stderr, "Unsupported codec\n");
            return 0;
        }

        err = avcodec_open2(ctx->codecctx, ctx->codec, NULL);
        if(av_err(err, "avcodec_open2")) return 0;

        ctx->w_ = ctx->codecctx->width;
        ctx->h_ = ctx->codecctx->height;
        ctx->vframe_ = avcodec_alloc_frame();
        if(!ctx->vframe_) {
            fprintf(stderr, "avcodec_alloc_frame failed\n");
            return 0;
        }

        avpicture_alloc(&(ctx->image), PIX_FMT_RGB24, ctx->w_, ctx->h_);
        ctx->img_convert_ctx = sws_getContext( ctx->w_, ctx->h_, ctx->codecctx->pix_fmt, ctx->w_, ctx->h_, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
        return 1;
    }

    static int cmp(const void *pi1, const void *pi2, void* data)
    {
        int i1 = *(const int *)pi1;
        int i2 = *(const int *)pi2;
        repere_video* ctx = (repere_video*) data;
        return ctx->local_index[i1].byte < ctx->local_index[i2].byte ? -1 : ctx->local_index[i1].byte > ctx->local_index[i2].byte ? 1 : 0;
    }

    static int load_index(repere_video* ctx, const char *fname)
    {
        int *ii;
        int i;
        frame_index *tmp;
        char buf[4096];
        FILE *fd;
        sprintf(buf, "Error opening %s", fname);
        fd = fopen(fname, "r");
        if(!fd) {
            perror(buf);
            return 0;
        }

        ctx->local_index_size = 0;
        ctx->local_index_allocated = 16384;
        ctx->local_index = (frame_index*) malloc(ctx->local_index_allocated * sizeof(ctx->local_index[0]));

        while(fgets(buf, sizeof(buf), fd)) {
            char *p = buf;
            char *q;
            while(*p == ' ' || *p == '\t' || *p == '\r')
                p++;
            if(!*p || *p == '\n')
                break;
            while(*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
                p++;
            while(*p == ' ' || *p == '\t' || *p == '\r')
                p++;
            if(ctx->local_index_size == ctx->local_index_allocated) {
                ctx->local_index_allocated += 16384;
                ctx->local_index = (frame_index*) realloc(ctx->local_index, ctx->local_index_allocated * sizeof(ctx->local_index[0]));      
            }

            tmp = ctx->local_index + ctx->local_index_size;
            if(!*p || *p == '\n')
                break;
            tmp->frame = *p++;
            while(*p == ' ' || *p == '\t' || *p == '\r')
                p++;
            q = p;
            while(*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
                p++;
            if(!*p || *p == '\n')
                break;
            *p++ = 0;
            tmp->byte = strtol(q, 0, 10);
            while(*p == ' ' || *p == '\t' || *p == '\r')
                p++;
            tmp->time = strtod(p, 0);
            tmp->previous_frame = -1;
            ctx->local_index_size++;
        }
        fclose(fd);

        ii = (int*) malloc(ctx->local_index_size * sizeof(int));
        for(i=0; i<ctx->local_index_size; i++)
            ii[i] = i;
        qsort_r(ii, ctx->local_index_size, sizeof(int), cmp, ctx);
        for(i=1; i<ctx->local_index_size; i++)
            ctx->local_index[ii[i]].previous_frame = ii[i-1];
        free(ii);
        return 1;
    }

    static int ff_read(repere_video* ctx)
    {
        for(;;) {
            AVPacket packet;
            int err = av_read_frame( ctx->formatctx, &packet );
            if(av_err(err, "av_read_frame")) return 0;
            if(packet.stream_index == ctx->vstream) {
                int frame_finished;
                avcodec_decode_video2(ctx->codecctx, ctx->vframe_, &frame_finished, &packet);
                av_free_packet(&packet);
                if(frame_finished) {
                    sws_scale(ctx->img_convert_ctx, (const uint8_t * const*)ctx->vframe_->data, ctx->vframe_->linesize, 0, ctx->h_, ctx->image.data, ctx->image.linesize);
                    return 1;
                }
            } else { // fixed memory leak
                av_free_packet(&packet);
            }
        } 
        return 1;
    }       

    // warning: ids start to 1
    static double repere_decode_frame(repere_video* ctx, int id)
    {
        int i = id-1;
        int key, j;
        if(ctx->reopen) {
            close_mpeg(ctx);
            if(!open_mpeg(ctx, ctx->filename)) return -1;
        }
        while(i>=0 && (i>=id || ctx->local_index[i].frame != 'I'))
            i = ctx->local_index[i].previous_frame;
        key = i+1;
        if(i != -1)
            i = ctx->local_index[i].previous_frame;
        av_seek_frame(ctx->formatctx, -1,  i >= 0 ? ctx->local_index[i].byte : 0, AVSEEK_FLAG_BYTE | AVSEEK_FLAG_ANY);

        do {
            if(!ff_read(ctx)) return -1;
        } while(!ctx->vframe_->key_frame);
        for(j = key; j != id; j++)
            if(!ff_read(ctx)) return -1;
        return ctx->local_index[id - 1].time;
    }

    static void repere_close(repere_video* ctx) {
        if(ctx->formatctx != NULL) close_mpeg(ctx);
        if(ctx->local_index != NULL) free(ctx->local_index);
        if(ctx->filename != NULL) free(ctx->filename);
        free(ctx);
    }

    static repere_video* repere_open(const char* mpg_filename, const char* index_filename) {
        repere_video *ctx = (repere_video*) calloc(sizeof(repere_video), 1);
        if(!load_index(ctx, index_filename)) {
            repere_close(ctx);
            return NULL;
        }
        ctx->filename = strdup(mpg_filename);
        ctx->reopen = 1; // force reopen at each frame
        if(!open_mpeg(ctx, ctx->filename)) {
            repere_close(ctx);
            return NULL;
        }
        return ctx;
    }
    
};
