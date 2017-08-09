﻿/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#if defined(HAVE_CONFIG_H)
#include "mediastreamer-config.h"
#endif

#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/waveheader.h"
#include "mediastreamer2/msticker.h"


static int player_close(MSFilter *f, void *arg);

struct _PlayerData{
    int fd;
    MSPlayerState state;
    int rate;
    int bitsize;
    int nchannels;
    int codectype;
    int hsize;
    int loop_after;
    int pause_time;
    int count;
    int playtime;
    int datalen;
    bool_t swap;
    int special_case;
    bool_t first_load;
};

typedef struct _PlayerData PlayerData;

static void player_init(MSFilter *f){
    PlayerData *d=ms_new(PlayerData,1);
    d->fd=-1;
    d->state=MSPlayerClosed;
    d->swap=FALSE;
    d->rate=8000;
    d->bitsize = 16;
    d->nchannels=1;
    d->codectype = -1;
    d->hsize=0;
    d->loop_after=-1; /*by default, don't loop*/
    d->pause_time=0;
    d->count=0;
    d->playtime=0;
    d->datalen=0;
    d->special_case=0;
    f->data=d;
    d->first_load=TRUE;
}

static int read_wav_header(PlayerData *d){
    char header1[sizeof(riff_t)];
    char header2[sizeof(format_t)];
    char header3[sizeof(data_t)];
    int count;

    riff_t *riff_chunk=(riff_t*)header1;
    format_t *format_chunk=(format_t*)header2;
    data_t *data_chunk=(data_t*)header3;

    unsigned long len=0;

    len = read(d->fd, header1, sizeof(header1)) ;
    if (len != sizeof(header1)){
        goto not_a_wav;
    }

    if (0!=strncmp(riff_chunk->riff, "RIFF", 4) || 0!=strncmp(riff_chunk->wave, "WAVE", 4)){
        goto not_a_wav;
    }

    len = read(d->fd, header2, sizeof(header2)) ;
    if (len != sizeof(header2)){
        ms_warning("Wrong wav header: cannot read file");
        goto not_a_wav;
    }

    d->rate=le_uint32(format_chunk->rate);
    d->nchannels=le_uint16(format_chunk->channel);
    d->codectype=le_uint16(format_chunk->type);
    d->bitsize=le_uint16(format_chunk->bitpspl);

#ifdef WORDS_BIGENDIAN
    format_chunk->len=le_uint32(format_chunk->len);
#endif

    if (format_chunk->len-0x10>0)
    {
        lseek(d->fd,(format_chunk->len-0x10),SEEK_CUR);
    }

    d->hsize=sizeof(wave_header_t)-0x10+format_chunk->len;

    len = read(d->fd, header3, sizeof(header3)) ;
    if (len != sizeof(header3)){
        ms_warning("Wrong wav header: cannot read file");
        goto not_a_wav;
    }
    count=0;
    while (strncmp(data_chunk->data, "data", 4)!=0 && count<30)
    {
        ms_warning("skipping chunk=%s len=%i", data_chunk->data, data_chunk->len);
        lseek(d->fd,data_chunk->len,SEEK_CUR);
        count++;
        d->hsize=d->hsize+len+data_chunk->len;

        len = read(d->fd, header3, sizeof(header3)) ;
        if (len != sizeof(header3)){
            ms_warning("Wrong wav header: cannot read file");
            goto not_a_wav;
        }
    }
    #ifdef WORDS_BIGENDIAN
    if (le_uint16(format_chunk->blockalign)==le_uint16(format_chunk->channel) * 2)
        d->swap=TRUE;
    #endif
    d->datalen = data_chunk->len;
    d->playtime = data_chunk->len/format_chunk->bps;// roughly estimate in sec(int)
    return 0;

    not_a_wav:
        /*rewind*/
        lseek(d->fd,0,SEEK_SET);
        d->hsize=0;
        return -1;
}

static int player_open(MSFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    int fd;
    const char *file=(const char*)arg;

    if (d->fd>=0){
        player_close(f,NULL);
    }
    if ((fd=open(file,O_RDONLY|O_BINARY))==-1){
        ms_warning("Failed to open %s",file);
        return -1;
    }
    d->state=MSPlayerPaused;
    d->fd=fd;

    if (strstr(file,".wav") || strstr(file,".WAV")){
        if (read_wav_header(d)!=0)
            printf("File %s has .wav extension but wav header could be found.\n",file);
    }else if (strstr(file,".svm")){
        d->codectype = 0; //uknow codec not .wav file
    }else{
        printf("not .wav or .svm file\n");
    }
    if(d->codectype == PCM && d->bitsize ==8)
        d->codectype = OFFSET8BIT;

    Castor3snd_reinit_for_diff_rate(d->rate,d->bitsize);//chane to diff sampling rate
    ms_message("%s opened: rate=%i,channel=%i",file,d->rate,d->nchannels);
    return 0;
}

static int player_start(MSFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    if (d->state==MSPlayerPaused)
        d->state = MSPlayerPlaying;
    else{
        d->state = MSdummyPlaying;
        printf("MSdummyPlaying scilent\n");
    }
    return 0;
}

static int player_stop(MSFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    ms_filter_lock(f);
    if (d->state!=MSPlayerClosed){
        d->state=MSPlayerPaused;
        lseek(d->fd,d->hsize,SEEK_SET);
    }
    ms_filter_unlock(f);
    return 0;
}

static int player_pause(MSFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    ms_filter_lock(f);
    if (d->state==MSPlayerPlaying){
        d->state=MSPlayerPaused;
    }
    ms_filter_unlock(f);
    return 0;
}

static int player_close(MSFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    player_stop(f,NULL);
    if (d->fd>=0)   close(d->fd);
    d->fd=-1;
    d->state=MSPlayerClosed;
    return 0;
}

static int player_get_state(MSFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    *(int*)arg=d->state;
    return 0;
}

static void player_uninit(MSFilter *f){
    PlayerData *d=(PlayerData*)f->data;
    if (d->fd>=0) player_close(f,NULL);
    ms_free(d);
}

static void swap_bytes(unsigned char *bytes, int len){
    int i;
    unsigned char tmp;
    for(i=0;i<len;i+=2){
        tmp=bytes[i];
        bytes[i]=bytes[i+1];
        bytes[i+1]=tmp;
    }
}

static void player_process(MSFilter *f){
    PlayerData *d=(PlayerData*)f->data;
    int nsamples=(f->ticker->interval*d->rate*d->nchannels)/1000;
    int bytes;
    /*send an even number of samples each tick. At 22050Hz the number of samples per 10 ms chunk is odd.
    Odd size buffer of samples cause troubles to alsa. Fixing in alsa is difficult, so workaround here.
    */
    if (nsamples & 0x1 ) { //odd number of samples
        if (d->count & 0x1 )
            nsamples++;
        else
            nsamples--;
    }
    if (d->special_case == 0 && d->first_load)
        bytes=3*nsamples;
    else if (d->special_case == 1)
        bytes=20;
    else
        bytes=2*nsamples;
    
    d->count++;
    ms_filter_lock(f);
    if (d->state==MSPlayerPlaying){
        int err;
        mblk_t *om=allocb(bytes,0);
        if (d->pause_time>0){
            err=bytes;
            memset(om->b_wptr,0,bytes);
            d->pause_time-=f->ticker->interval;
        }else{
            err=read(d->fd,om->b_wptr,bytes);
            if (d->swap) swap_bytes(om->b_wptr,bytes);
        }
        if (err>=0){
            if (err!=0){
                if (err<bytes)
                    memset(om->b_wptr+err,0,bytes-err);
                om->b_wptr+=bytes;
                ms_queue_put(f->outputs[0],om);
            }else freemsg(om);
            if (err<bytes){
                lseek(d->fd,d->hsize,SEEK_SET);
                if (d->special_case == 0 && d->first_load)
                    d->first_load=FALSE;
                /* special value for playing file only once */
                if (d->loop_after<0)
                {
                    d->state=MSPlayerEof;
                    d->pause_time=50000;//50ms buffer time
                    // d->state=MSPlayerPaused;
                    // ms_filter_unlock(f);
                    // return;
                }

                if (d->loop_after>=0){
                    d->pause_time=d->loop_after;
                }
            }
        }else{
            ms_warning("Fail to read %i bytes: %s",bytes,strerror(errno));
        }
    }
    if(d->state==MSPlayerEof){//add null time(data) prevent abnormal sound 
        int err;
        mblk_t *om=allocb(bytes,0);
        if (d->pause_time>0){
            err=bytes;
            memset(om->b_wptr,0,bytes);
            om->b_wptr+=bytes;
            d->pause_time-=f->ticker->interval;
            ms_queue_put(f->outputs[0],om);
        }else{
            ms_filter_unlock(f);
            d->state=MSPlayerPaused;
            return;
        }
    }
    if(d->state == MSdummyPlaying){
        mblk_t *om=allocb(bytes,0);
        memset(om->b_wptr,0,bytes);
        om->b_wptr+=bytes;
        ms_queue_put(f->outputs[0],om);
    }
    ms_filter_unlock(f);
}

static int player_get_sr(MSFilter *f, void*arg){
    PlayerData *d=(PlayerData*)f->data;
    *((int*)arg)=d->rate;
    return 0;
}

static int player_get_codectype(MSFilter *f, void*arg){
    PlayerData *d=(PlayerData*)f->data;
    *((int*)arg)=d->codectype;
    return 0;
}

static int player_get_playtime(MSFilter *f, void*arg){
    PlayerData *d=(PlayerData*)f->data;
    *((int*)arg)=d->playtime;
    return 0;
}

static int player_get_datalen(MSFilter *f, void*arg){
    PlayerData *d=(PlayerData*)f->data;
    int ratio = 1;
    if(d->codectype == ULAW ||d->codectype == ALAW || d->codectype == OFFSET8BIT)
        ratio = 2;
    *((int*)arg)=d->datalen*ratio;
    //printf("d->codectype = %d  playlenth:d->datalen*ratio = %d*%d\n",d->codectype,d->datalen,ratio);
    return 0;
}

static int player_loop(MSFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    d->loop_after=*((int*)arg);
    return 0;
}

static int player_eof(MSFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    if (d->fd<0 && d->state==MSPlayerClosed)
        *((int*)arg) = TRUE; /* 1 */
    else
        *((int*)arg) = FALSE; /* 0 */
    return 0;
}

static int player_get_nch(MSFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    *((int*)arg)=d->nchannels;
    return 0;
}

static int play_set_special_case(MSFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    d->special_case=*((int*)arg);
    d->first_load=TRUE;
    return 0;
}

static MSFilterMethod player_methods[]={
    {   MS_FILE_PLAYER_OPEN,    player_open },
    {   MS_FILE_PLAYER_START,   player_start    },
    {   MS_FILE_PLAYER_STOP,    player_stop },
    {   MS_FILE_PLAYER_CLOSE,   player_close    },
    {   MS_FILTER_GET_CODEC_TYPE, player_get_codectype}, 
    {   MS_FILTER_GET_PLAY_TIME, player_get_playtime}, 
    {   MS_FILTER_GET_DATA_LENGTH, player_get_datalen}, 
    {   MS_FILTER_GET_NCHANNELS, player_get_nch },
    {   MS_FILE_PLAYER_LOOP,    player_loop },
    {   MS_FILE_PLAYER_DONE,    player_eof  },
    {   MS_FILE_PLAYER_SET_SPECIAL_CASE, play_set_special_case},
    {   MS_FILTER_GET_SAMPLE_RATE, player_get_sr},    
    /* this wav file player implements the MSFilterPlayerInterface*/
    { MS_PLAYER_OPEN , player_open },
    { MS_PLAYER_START , player_start },
    { MS_PLAYER_PAUSE, player_pause },
    { MS_PLAYER_CLOSE, player_close },
    { MS_PLAYER_GET_STATE, player_get_state },
    {   0,          NULL        }
};

#ifdef WIN32

MSFilterDesc ms_file_player_desc={
    MS_FILE_PLAYER_ID,
    "MSFilePlayer",
    N_("Raw files and wav reader"),
    MS_FILTER_OTHER,
    NULL,
    0,
    1,
    player_init,
    NULL,
    player_process,
    NULL,
    player_uninit,
    player_methods
};

#else

MSFilterDesc ms_file_player_desc={
    .id=MS_FILE_PLAYER_ID,
    .name="MSFilePlayer",
    .text=N_("Raw files and wav reader"),
    .category=MS_FILTER_OTHER,
    .ninputs=0,
    .noutputs=1,
    .init=player_init,
    .process=player_process,
    .uninit=player_uninit,
    .methods=player_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_file_player_desc)
