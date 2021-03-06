#include "Protocol.h"

VOID Init(PQueueInfo pQueueInfo)
{
	pQueueInfo->Front = 0;
	pQueueInfo->Rear = 0;
}
QueueLengthType GetQueueLength(PQueueInfo pQueueInfo)
{
	return (pQueueInfo->Rear - pQueueInfo->Front + MAXSIZE) % MAXSIZE;
}
BOOL AddQueue(PQueueInfo pQueueInfo, QueueDataType qValue)
{
    BOOL bState;
	if((pQueueInfo->Rear + 1) % MAXSIZE == pQueueInfo->Front)
	{
		bState = FALSE;
	}
    else
    {
    	pQueueInfo->queuedata[pQueueInfo->Rear] = qValue;
    	pQueueInfo->Rear = (pQueueInfo->Rear + 1) % MAXSIZE;
    	bState = TRUE;
    }
    return bState;
}
BOOL DeleteQueue(PQueueInfo pQueueInfo, QueueDataType *pqValue)
{
    BOOL bState;
	if(pQueueInfo->Front == pQueueInfo->Rear)
	{
		bState = FALSE;
	}
    else
    {
    	*pqValue = pQueueInfo->queuedata[pQueueInfo->Front];
    	pQueueInfo->Front = (pQueueInfo->Front + 1) % MAXSIZE;
        bState = TRUE;
    }
	return bState;
}
