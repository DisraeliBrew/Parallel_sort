

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#define MAX_BUCKET 10   /*桶容器的数量*/
#define RANGE_BUCKET 10 /*每个桶容器的增量范围*/
#define MAX_NUMBERS 200 /*用于测试排序的数据总量*/


//定义数据节点，包含数据和指向下一个节点的指针
typedef struct _node {
    int data;
    struct _node * next;
}
node;



//定义数据结构，传递至线程执行函数里面
struct thread_data{
   int  thread_id;//当前线程的ID
   node *in;//当前线程需要处理的单链表
};

//声明线程，每个桶容器对应一个线程
pthread_t threads[MAX_BUCKET];

//声明传递至线程执行函数的结构体数组
struct thread_data thread_data_array[MAX_BUCKET];

typedef struct entry
{
    int key;
    int* record;
};

struct entry *entryArray;

//声明桶排序函数
void bucket_sort(int array[],int arraySize);

//声明每个线程的处理函数
void *thread_bucket_sort(void *in);

//声明插入排序函数
node *insert_sort(node *list);

//声明计算桶容器下标的函数
int returnBucketIndex(int data);


//声明桶容器，用于数据分配以及排序
node ** all_buckets;
//声明桶容器，用于在线程中排序完成后更新
node ** all_buckets_t;

    

//桶排序函数定义
void bucket_sort(int array[], int arraySize)
{
    
    int i,j;
    
    //分配内存
    all_buckets = (node **)malloc(sizeof(node *)*MAX_BUCKET);
    all_buckets_t = (node **)malloc(sizeof(node *)*MAX_BUCKET);
    //初始化
    for(i = 0; i<MAX_BUCKET; i++)
    {
        all_buckets[i] = NULL;
        all_buckets_t[i] = NULL;
    }

    //将待排序的数据按照一定的规律分配到每个桶形容器里
    for(i = 0; i<arraySize; i++)
    {
        node *cur;
        //计算当前数据映射到哪一个桶
        int bucket_index = returnBucketIndex(array[i]);
        cur = (node *)malloc(sizeof(node));
        cur->data = array[i];
        cur->next = all_buckets[bucket_index];
        all_buckets[bucket_index] = cur;
    }

    //打印出结果查看每个桶容器里的数据
    for(i = 0; i<MAX_BUCKET; i++)
    {
        node *temp = all_buckets[i];
        if(temp != NULL)
        {
            printf("当前是第%d个桶容器,包含的数据有：\n",i);
            while(temp!=NULL)
            {
                printf("%d ",temp->data);
                temp = temp->next;
            }
            printf("\n");
        }
    }

    printf("\n");
    printf("**********华丽的分割线**********\n",i);
    printf("\n");
    printf("排序之后每个桶容器里面的数据如下:\n");
    //用其他排序方法对每个桶容器里面的数据进行排序,这里选择的是插入排序
    for(i = 0; i<MAX_BUCKET; i++)
    {
        thread_data_array[i].thread_id = i;
        thread_data_array[i].in = all_buckets[i];
        int rc = pthread_create(&threads[i], NULL, thread_bucket_sort, (void *) &thread_data_array[i]);
        if (rc)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            return;
        }
    }
    
    for(i=0; i<MAX_BUCKET; i++)
    {
        int rc = pthread_join(threads[i], NULL);
        if (rc)
        {
            printf("ERROR; return code from pthread_join() is %d\n", rc);
            return;
        }
    }

    //打印出结果查看每个桶容器里的数据
    printf("\n最终排序好的数组如下：\n");
    //将每个桶容器里面的数据取出来重新放回数组里
    for(j =0, i = 0; i < MAX_BUCKET; i++) {    
        node *temp_node = all_buckets_t[i];
        while(temp_node != NULL)
        {
            array[j++] = temp_node->data;
            printf("%d  ", temp_node->data);
            temp_node = temp_node->next;
        }
    }
    printf("\n数组总长度为%d\n",j);

    
    //释放内存
    for(i = 0; i<MAX_BUCKET; i++)
    {
        node *head = all_buckets[i];
        while(head != NULL)
        {
            node *temp;
            temp = head;
            head = head->next;
            free(temp);
        }
    }
    free(all_buckets_t);
    free(all_buckets);
}

//每个线程执行的函数
void *thread_bucket_sort(void *in)
{
    struct thread_data *strcut_data = (struct thread_data *)in;
    node *list = (strcut_data->in);
    node *out = insert_sort(list);
    node *temp = out;

    //分配内存
    all_buckets_t[strcut_data->thread_id] = (node *)malloc(sizeof(node));
    all_buckets_t[strcut_data->thread_id] = out;
    if(list != NULL)
    {
        printf("当前是第%d个桶容器,包含的数据有：\n",strcut_data->thread_id);
        while(temp!=NULL)
        {
            printf("%d ",temp->data);
            temp = temp->next;
        }
        printf("\n");
    }
    //退出线程
    pthread_exit(NULL);
}

//插入排序函数定义
node *insert_sort(node *list)
{
    //如果当前桶容器里没有数据或者数据个数只有1，则不需要排序
    if(list == NULL || list->next == NULL)
    {
        return list;
    }
    
    //指向其他元素
    node *k = list->next;
    //构建一个有序链表，只有一个元素
    node *nodeList = list;
    nodeList->next = NULL;
    
    while(k != NULL) 
    { 
        node *ptr;
        //如果待插入的第一个数小于当前有序链表的第一个（即最小值）,则在之前插入
        if(nodeList->data > k->data)  { 
            node *tmp;
            tmp = k;  
            k = k->next; 
            tmp->next = nodeList;
            nodeList = tmp; 
            continue;
        }

        //找到插入的位置
        for(ptr = nodeList; ptr->next != NULL; ptr = ptr->next) {
            if(ptr->next->data > k->data) break;
        }

        //插入操作，分两种情况
        if(ptr->next!=NULL)
        {  
            node *tmp;
            tmp = k;  
            k = k->next; 
            tmp->next = ptr->next;
            ptr->next = tmp; 
        }
        else
        {
            ptr->next = k;  
            k = k->next;  
            ptr->next->next = NULL; 
        }
    }
    return nodeList;
}

//计算桶容器下标的函数定义
int returnBucketIndex(int data)
{
    int ret = data/RANGE_BUCKET;
    return ret;
}

void main(int argc,char *argv[])
{
int i, size;
int fdin, fdout;
char *src, *dst;
struct stat statbuf;
int mode = 0x0777;

 printf("line 257\n");

     /* open the input file */
 if ((fdin = open (argv[1], O_RDONLY)) < 0)
   {printf("can't open %s for reading", argv[1]);
    return 0;
   }

   printf("line 264\n");

 /* open/create the output file */
//  if ((fdout = open (argv[2], O_RDWR | O_CREAT | O_TRUNC, mode )) < 0)//edited here
//    {printf ("can't create %s for writing", argv[2]);
//     return 0;
//    }
   printf("line 271\n");
 /* find size of input file */
 if (fstat (fdin,&statbuf) < 0)
   {printf ("fstat error");
    return 0;
   }
printf("line 277\n");
 /* go to the location corresponding to the last byte */
//  if (lseek (fdout, statbuf.st_size - 1, SEEK_SET) == -1)
//    {printf ("lseek error");
//     return 0;
//    }
printf("line 283\n");
 /* write a dummy byte at the last location */
//  if (write (fdout, "", 1) != 1)
//    {printf ("write error");
//      return 0;
//    }

   void* mapPtr;

 /* mmap the input file */
 if ((src = mmap (0, statbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0))
   == (mapPtr) -1)
   {printf ("mmap error for input");
    return 0;
   }
   printf("line 298\n");
 entryArray = (struct entry*)malloc(sizeof(struct entry) * 100000);
//entryArray = malloc(sizeof(struct entry) * 100);
//    int *arr = malloc(10* sizeof(int));
//    arr[0] = 5;

//    struct entry *ptr;
//    ptr = (struct entry*)malloc(sizeof(struct entry) * 100);
//   ptr[i].key = *(src+i);
//     ptr[i].record = *(src+i+4);
printf("line 309\n");
printf("%d\n",statbuf.st_size);
   for(int i =0 ; i<statbuf.st_size;i = i+100){
    
    entryArray[i].key = (int) *(src+i);
    entryArray[i].record =  (src+i+4);
    printf("key: %d, record: %d\n",entryArray[i].key,entryArray[i].record[0]);
   }
printf("line 316\n");
int first = *src;
printf("first key: %d \n", first);
printf("first record: %d",*(src+4));
//    for(int i=0;i<statbuf.st_size/100;i++){
//     printf("Key is : %d, value is %d",entryArray[i].key,entryArray[i].record[0]);
//    }

 /* mmap the output file */
//  if ((dst = mmap (0, statbuf.st_size, PROT_READ | PROT_WRITE,
//    MAP_SHARED, fdout, 0)) == (mapPtr) -1)
//    {printf ("mmap error for output");
//     return 0;
//    }

    return;
}
