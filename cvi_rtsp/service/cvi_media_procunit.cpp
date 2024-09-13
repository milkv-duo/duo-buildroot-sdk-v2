#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/prctl.h>
#include "cvi_media_procunit.h"
//#include "cvi_param.h"

typedef struct _MediaListHead_S {
	struct _MediaListHead_S *Prev;
	struct _MediaListHead_S *Next;
} MEIDALISTHEAD_S;

typedef struct _MEDIAUNITPROCNODE_S {
	void *data;
	MEIDALISTHEAD_S HeadNode;
} MEDIAUNITPROCNODE_S;

typedef struct {
	CVI_S32 venc;
	void *pram;
	media_handler_func handler_func;
	media_malloc_func malloc_func;
	media_relase_func relase_func;
	pthread_t pthread_id;
	CVI_S32 runstatus;
	pthread_mutex_t mutex;
	int listsize;
	MEDIAUNITPROCNODE_S head;
} MEDIAPROCUNIN_INFO;

static MEDIAPROCUNIN_INFO *s_mediaUnit = NULL;
static CVI_S32 s_vencChnNum = 0;


#define container_of(ptr,type,member) ({ \
    const typeof(((type *)0)->member) *mptr=ptr;\
    (type *)((char *)mptr-offsetof(type,member)); \
})

#define MediaList_ForEach(pos,head,member) \
    if(head)                            \
    for(pos=container_of(head->Next,typeof(*pos),member);pos&&(&(pos->member))!=head;pos=container_of(pos->member.Next,typeof(*pos),member))

int cvi_MeidaCommonListInit(MEIDALISTHEAD_S *Head)
{
    if(!Head)
    {
        return -1;
    }
    Head->Prev=Head->Next=Head;
    return 0;
}

static int MeidaCommonListInsert(MEIDALISTHEAD_S *NewNode,MEIDALISTHEAD_S *Prev,MEIDALISTHEAD_S *Next)
{
    Next->Prev=NewNode;
    NewNode->Next=Next;
    NewNode->Prev=Prev;
    Prev->Next=NewNode;
    return 0;
}


static int MeidaCommonListTailerInsert(MEIDALISTHEAD_S *NewNode,MEIDALISTHEAD_S *Head)
{
    if(!Head||!NewNode)
        return -1;
    MeidaCommonListInsert(NewNode,Head->Prev,Head);
    return 0;
}
#if 0
static int MeidaCommonListHeadInsert(MEIDALISTHEAD_S *NewNode,MEIDALISTHEAD_S *Head)
{
    if(!Head||!NewNode)
        return -1;
    MeidaCommonListInsert(NewNode,Head,Head->Next);
    return 0;
}
#endif

static int _CommonListDelete(MEIDALISTHEAD_S *Prev,MEIDALISTHEAD_S *Next)
{
    Prev->Next = Next;
    Next->Prev = Prev;
    return 0;
}


static int MeidaCommonListDelete(MEIDALISTHEAD_S *Entry)
{

    if(!Entry) {
        return -1;
    }
    _CommonListDelete(Entry->Prev,Entry->Next);
    return 0;
}
#if 0
static int MeidaCommonListMove(MEIDALISTHEAD_S *Entry,MEIDALISTHEAD_S *Head)
{
    if(!Entry||!Head) {
        return -1;
    }
    MeidaCommonListDelete(Entry);
    MeidaCommonListHeadInsert(Entry,Head);
    return 0;
}
#endif
static int MediaCommonListEmpty(MEIDALISTHEAD_S *Head)
{

    if(!Head) {
        return CVI_FAILURE;
    }
    if(Head->Next == Head) {
        return CVI_FAILURE;
    }
    return CVI_SUCCESS;
}

#define MeidaCommonListHeadPop(PopPoint,Head,member) ({  \
        PopPoint=container_of(Head->Next,typeof(*PopPoint),member);    \
        if(Head->Next == Head) \
            PopPoint=NULL;          \
        MeidaCommonListDelete(Head->Next);              \
})



CVI_S32 CVI_MEDIA_ListPushBack(CVI_S32 venc,void *pstream)
{

    if(!pstream ) {
        return CVI_FAILURE;
    }
    if(venc > s_vencChnNum || !s_mediaUnit) {
        return CVI_FAILURE;
    }
    if(s_mediaUnit[venc].runstatus == 0) {
        return CVI_FAILURE;
    }
    MEDIAUNITPROCNODE_S * Head = &s_mediaUnit[venc].head;
    MEDIAUNITPROCNODE_S * newnode = NULL;
    newnode = (MEDIAUNITPROCNODE_S *)malloc(sizeof(MEDIAUNITPROCNODE_S));
    if(newnode == NULL) {
        goto exit;
    }
    newnode->data = NULL;
    if(s_mediaUnit[venc].malloc_func) {
        if(s_mediaUnit[venc].malloc_func(&newnode->data,pstream) != CVI_SUCCESS) {
            goto exit;
        }
    }
    if(s_mediaUnit[venc].listsize >= MEDIAPROCUNIT_MAXDATASIZE) {
        void *psteam = NULL;
        CVI_MEDIA_ListPopFront(venc,&psteam);
        if(psteam) {
            if(s_mediaUnit[venc].relase_func) {
                s_mediaUnit[venc].relase_func(&psteam);
            }
            psteam = NULL;
        }
    }
    pthread_mutex_lock(&s_mediaUnit[venc].mutex);
    MeidaCommonListTailerInsert(&newnode->HeadNode,&Head->HeadNode);
    s_mediaUnit[venc].listsize ++;
    pthread_mutex_unlock(&s_mediaUnit[venc].mutex);
    return CVI_SUCCESS;
exit:
    if(newnode) {
        if(newnode->data) {
            free(newnode->data);
        }
        free(newnode);
    }
    return CVI_FAILURE;
}

CVI_S32 CVI_MEDIA_ListPopFront(CVI_S32 venc,void **pstream)
{
    if(!pstream) {
        return CVI_FAILURE;
    }
    if(venc > s_vencChnNum || !s_mediaUnit) {
        return CVI_FAILURE;
    }
    if(MediaCommonListEmpty(&s_mediaUnit[venc].head.HeadNode) != CVI_SUCCESS) {
        return CVI_FAILURE;
    }
    MEDIAUNITPROCNODE_S *_tmppoint = NULL;
    pthread_mutex_lock(&s_mediaUnit[venc].mutex);
    MeidaCommonListHeadPop(_tmppoint,(&s_mediaUnit[venc].head.HeadNode),HeadNode);
    if(_tmppoint) {
        s_mediaUnit[venc].listsize --;
    }
    pthread_mutex_unlock(&s_mediaUnit[venc].mutex);
    if(_tmppoint) {
        *pstream = _tmppoint->data;
        free(_tmppoint);
    } else {
        return CVI_FAILURE;
    }
    return CVI_SUCCESS;
}

void *CVI_MEIDA_EventHander(void *args)
{
    //CVI_MEIDA_EventHander
    MEDIAPROCUNIN_INFO *pstMediaProcUnit = (MEDIAPROCUNIN_INFO *)args;
    void *psteam = NULL;
    CVI_CHAR TaskName[64] = {0};

    sprintf(TaskName, "chn%dMediaEventHander", pstMediaProcUnit->venc);
    prctl(PR_SET_NAME, TaskName, 0, 0, 0);
    while(pstMediaProcUnit->runstatus) {
        //
        if(CVI_MEDIA_ListPopFront(pstMediaProcUnit->venc,&psteam) == CVI_SUCCESS) {
            if(psteam) {
                if(pstMediaProcUnit->handler_func) {
                    pstMediaProcUnit->handler_func(pstMediaProcUnit->venc,pstMediaProcUnit->pram,psteam);
                }
                if(s_mediaUnit[pstMediaProcUnit->venc].relase_func) {
                    s_mediaUnit[pstMediaProcUnit->venc].relase_func(&psteam);
                }
                psteam = NULL;
            }
        }
        usleep(5*1000);
    }
    return 0;
}

CVI_S32 CVI_MEDIA_ProcUnitInit(CVI_S32 num,void *pstparm,media_handler_func phandler,media_malloc_func pmalloc,media_relase_func prelase)
{

    CVI_S32 i = 0 ;
    if(num == 0 ) {
        return CVI_FAILURE;
    }
    if(s_vencChnNum == 0 ) {
        s_mediaUnit = (MEDIAPROCUNIN_INFO *)malloc(num * sizeof(MEDIAPROCUNIN_INFO));
        if(!s_mediaUnit) {
            return CVI_FAILURE;
        }
        s_vencChnNum = num;
        for(i = 0;i< num;i++) {
            s_mediaUnit[i].venc = i;
            s_mediaUnit[i].pram = pstparm;
            s_mediaUnit[i].handler_func = phandler;
            s_mediaUnit[i].malloc_func = pmalloc;
            s_mediaUnit[i].relase_func = prelase;
            s_mediaUnit[i].listsize = 0;
            pthread_mutex_init(&s_mediaUnit[i].mutex,NULL);
            s_mediaUnit[i].runstatus = 1;
            cvi_MeidaCommonListInit(&s_mediaUnit[i].head.HeadNode);
            if(pthread_create(&s_mediaUnit[i].pthread_id,NULL,CVI_MEIDA_EventHander,&s_mediaUnit[i]) != 0) {
                s_mediaUnit[i].runstatus = 0;
                pthread_mutex_destroy(&s_mediaUnit[i].mutex);
                goto exit;
            }
        }
    }
    return CVI_SUCCESS;
exit:
    if(s_mediaUnit) {
        free(s_mediaUnit);
    }
    return CVI_FAILURE;
}

CVI_S32 CVI_MEDIA_ProcUnitDeInit(void)
{
    //EXIT
    CVI_S32 i = 0;
    void *pstream = NULL;
    if(s_vencChnNum > 0) {
        if(s_mediaUnit) {
            for(i =0 ;i < s_vencChnNum;i++) {
                s_mediaUnit[i].runstatus = 0;
                pthread_join(s_mediaUnit[i].pthread_id,NULL);
                while(CVI_MEDIA_ListPopFront(i,&pstream) == CVI_SUCCESS) {
                    if(pstream) {
                        if(s_mediaUnit[i].relase_func) {
                            s_mediaUnit[i].relase_func(&pstream);
                        }
                        pstream = NULL;
                    }
                }
                pthread_mutex_destroy(&s_mediaUnit[i].mutex);
            }
            free(s_mediaUnit);
            s_mediaUnit = NULL;
        }
        s_vencChnNum = 0;
    }
    return CVI_SUCCESS;
}
