#ifndef __ILIBTHREADPOOL__
#define __ILIBTHREADPOOL__

typedef void* ILibThreadPool;
typedef void(*ILibThreadPool_Handler)(ILibThreadPool sender, void *var);

ILibThreadPool ILibThreadPool_Create();
void ILibThreadPool_AddThread(ILibThreadPool pool);
void ILibThreadPool_QueueUserWorkItem(ILibThreadPool pool, void *var, ILibThreadPool_Handler callback);
void ILibThreadPool_Destroy(ILibThreadPool pool);
int ILibThreadPool_GetThreadCount(ILibThreadPool pool);

#endif
