// Copyright 2014 tianxin. All Rights Reserved.
//
//to predict the word given the context.
//BUG: 1.error to deal with the blank line
//     2.end of line must be '\n', if have ' ' or '\t' then reeor.
//     3.sometimes meet "malloc memory corruption"
//     4.some times meet free error.
//     5.random series is same.
////////////////////////////////////////////////////////////////////////


// Copyright 2015-8-5 tianxin. All Rights Reserved.
//
//to predict context given current word.
//BUG: 1.error to deal with the blank line.
//
////////////////////////////////////////////////////////////////////////

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#define MAX_STRING 100
#define WORD_LENGTH 70

#define _DEBUG_

//typedef float real;
typedef double real;
struct vocab_possibility
{
    long long index;
    real possibility;
};
struct word_weight
{
    long long index;
    real weight;
};
struct vocab_word
{
    long long cn;
    int *point;
    char word[WORD_LENGTH];
    char *code;
    int codelen;
};
struct test_word
{
    char word[WORD_LENGTH];
    bool flag;
};
typedef struct
{
    char word[WORD_LENGTH];
} my_word;
//long long *max_possibility_index = NULL;
//real *possibility = NULL;

//the following string save the traning result through word2vec
struct vocab_word *vocab = NULL;
real *weight = NULL;
real *vec = NULL;


//save a test plan read by read_sentence() from test_file
struct test_word *sentence = NULL;
struct vocab_possibility *possibility = NULL;
long long vocab_size = 0;
long long vector_size = 0;
int  window = 2;
int NMAX_POSSIBILITY = 201;
double blank_rate = 0.05;
double iter = 5.0;
double learning_rate = 0.0002;
char word_file[MAX_STRING];
char weight_file[MAX_STRING];
char vector_file[MAX_STRING];
char test_file[MAX_STRING];

#define ITER_MAX 30.0
#define SET_SIZE_MAX 20

//int set_size = 40;
//long long max_possibility_index[NMAX_POSSIBILITY] = {0};
int ArgPos(char *str, int argc, char **argv);
int CountLength(FILE *fin);
//int predict(int, int, int, int);
int generate_random(int array[], int count, int length);
int Add_vector(int window_size, int vocab_size, int vector_size,my_word *word, real *add_vector);
real compute_word_possibility (char blank_word[WORD_LENGTH], char* , int window_size, int vocab_size, int vector_size);
int read_origin(char *word_file, char *weight_file, char *vector_file);
int read_sentence(FILE *pTest_file);
//int get_context_word(struct test_word* pSentence, int sentence_length, int blank_index, int window_size, char word[window_size][WORD_LENGTH]);
int get_context_word(struct test_word* pSentence, int sentence_length, int blank_index, int window_size, my_word *word);
int compare_weight( const void *a ,const void *b);
int commandlinde(int argc, char **argv);
int init(int blank_count, int **possibility_flag, my_word **correct_word, my_word **random_word ,int **blank_index, int sentence_length);
//int init(int blank_count, int *possibility_flag, my_word *correct_word, my_word *random_word ,int *blank_index, int sentence_length);
int free_fun(int *blank_index, my_word *correct_word, real *word_possibility_p0,  real *word_possibility_p1,struct word_weight* pWeight, int *random_word_index, my_word *random_word, int *possibility_flag);
int print_predicted_words(int blank_count, my_word *correct_word, int *blank_index, struct word_weight* pWeight, int *right_count_all);

real compute_p0(int blank_count, int *blank_index, int sentence_length, my_word *context_word, real *word_possibility_p0);
//real compute_p1(double iter, int blank_count, int *blank_index, int sentence_length, my_word *context_word,my_word *random_word, int *possibility_flag, real *word_possibility_p1, real *word_possibility_p0, real p0, struct word_weight* pWeight );
//real compute_p1(double iter, int blank_count, int *blank_index, int sentence_length, struct word_weight* pWeight);
real compute_p1(double iter, int blank_count, int *blank_index, int sentence_length, struct word_weight* pWeight, my_word* correct_word);

//int init_pWeight(struct word_weight* pWeight,  int blank_count);
int init_pWeight(struct word_weight* pWeight, int blank_count, int sentence_len);


real compute_partial_derivative(struct test_word* sentence, int sentence_length, int window, int blank_order,  struct word_weight* pWeight);
int search_word_index(char *word);

int min_max_norm(struct word_weight* pWeight, int sentence_index);

int main(int argc, char **argv)
{
    long long i;
    long long blank_count = 0;
    long long blank_total = 0;
    //long long right_count = 0;
    long long sentence_count = 0;
    double accurcy_rate = 0.0;
    // double epson = 0.0001;
    //  int predict_return = 0;
    //  char *new_line = NULL; //to save the '\n' in every end of line, then it is easy to compute the length of sentence.
    commandlinde( argc, argv);
    read_origin(word_file, weight_file, vector_file);
    FILE *pTest_file = NULL;
    pTest_file = fopen(test_file,"rb");
    if (pTest_file  == NULL)
    {
        printf("Test file not found\n");
        return -1;
    }
    // struct test_word *sentence = NULL;
    int *blank_index = NULL;
    // int *context_index = NULL;
    //reads sentence from test_file.
    srand((unsigned int)time(NULL));   /////////////////////////////////////////////////////////////////////////
    my_word *correct_word = NULL;


    real *word_possibility_p0 = NULL;
    real *word_possibility_p1 = NULL;


   /* my_word *context_word = NULL;
    context_word = (my_word*)malloc(window*sizeof(my_word));
    if(context_word == NULL)
    {
        printf("fail to allocate memory for context_word.\n");
        return -1;
    }*/


    int sentence_length = 0;
    char eof_flag;
    int *random_word_index = NULL;
    struct word_weight *pWeight = NULL;
    int *possibility_flag = NULL;
    my_word *random_word = NULL;
    //保存不同推荐集合对应的正确预测个数
    int right_count_all[SET_SIZE_MAX];

    for(i=0; i<SET_SIZE_MAX; i++)
        right_count_all[i] = 0;
    //   for(iter=5.0; iter<=ITER_MAX; iter+=5.0)
    //  {
    while( 1 )
    {
        eof_flag = fgetc(pTest_file);
        if(eof_flag == -1)
            break;
        else
            fseek(pTest_file, -1, SEEK_CUR);
        //   !feof(pTest_file)
        //        bool flag=0;
        //sentence_length = CountLength(pTest_file);
        //
        //read_sentence() will allocate memory for sentence
        sentence_length = read_sentence(pTest_file);
        if(sentence_length > 0)
            sentence_count++;
        blank_count = sentence_length * blank_rate;
        blank_total += blank_count;

        //initialize blank_index, random_word, correct_word.
        int init_return = init(blank_count, &possibility_flag, &correct_word, &random_word ,&blank_index, sentence_length);
        if(init_return == -2) continue;//if blank_count == 0, then read the next sentene
        //ignore abnormal value of blank_count
        //
        //printf("blank index generated randomly is:");
        // for(i=0; i<blank_count; i++)
        //printf("%d ",blank_index[i]);
        // printf("\n");
        //tag the blank word's flag to false;
        for(i=0; i<blank_count; i++)
            sentence[blank_index[i]].flag = false;
        ///////////////////////////////////////////////just for test if blank word tag is wright.
#ifdef _DEBUG_
        printf("\n");
        printf("sentence %lld's length is:%d\n",sentence_count, sentence_length);
        for(i=0; i<sentence_length; i++)
            printf("%s    %d\n",sentence[i].word, sentence[i].flag);
#endif // _DEBUG_
        //copy blank_index word to correct word to save
        for(i=0; i<blank_count; i++)
            strcpy(correct_word[i].word, sentence[blank_index[i]].word);


#ifdef _DEBUG_
   for(i=0; i<blank_count; i++)
   {
        printf("the original sentence, blanK word %lld:\n",i);
          compute_word_possibility (sentence[blank_index[i]].word, sentence[blank_index[i]-1].word,  2,  vocab_size, vector_size);
          compute_word_possibility (sentence[blank_index[i]].word, sentence[blank_index[i]+1].word,  2,  vocab_size, vector_size);
   }
#endif // _DEBUG_


        //initialize pWeight
        //pWeight = (struct word_weight*)malloc(blank_count*vocab_size * sizeof(struct word_weight));
         pWeight = (struct word_weight*)malloc(sentence_length*vocab_size * sizeof(struct word_weight));
        init_pWeight(pWeight,  blank_count, sentence_length);

//print the initial weigth of word in blank index.
#ifdef _DEBUG_
        /* for(i=0; i<blank_count; i++)
        {
        printf("weight %lld:",i);
        for(j = 0; j<vocab_size; j++)
        {
        printf("%f ",pWeight[i*vocab_size+j].weight);
        }
        printf("\n");
        }*/
#endif // _DEBUG_

//save initial random word index in vocabulary to compute p0;
        random_word_index = (int*)malloc(blank_count*sizeof(int));
        if(random_word_index == NULL)
        {
            printf("fail to allocate memory for random_word_index.\n");
            return -1;
        }
        generate_random(random_word_index, blank_count, vocab_size);
       // word_possibility_p0 = (real*)malloc(blank_count*sizeof(real));
       // word_possibility_p1 = (real*)malloc(blank_count*sizeof(real));

        for(i=0; i<blank_count; i++)
        {
            //copy random word to blank_index to compute p0
            strcpy(sentence[blank_index[i]].word, vocab[random_word_index[i]].word);
         //   word_possibility_p0[i] = 0.0;
        }

#ifdef _DEBUG_
   for(i=0; i<blank_count; i++)
   {
        printf("the changed sentence, blanK word %lld:\n",i);
          compute_word_possibility (sentence[blank_index[i]].word, sentence[blank_index[i]-1].word,  2,  vocab_size,  vector_size);
          compute_word_possibility (sentence[blank_index[i]].word, sentence[blank_index[i]+1].word,  2,  vocab_size,  vector_size);
   }
#endif // _DEBUG_

#ifdef _DEBUG_
        /*  for(i=0; i<blank_count; i++)
        {
        printf("the blank index %d random word is %s\n", blank_index[i], vocab[random_word_index[i]].word);
        }*/
#endif // _DEBUG_
//  char context_word[window][WORD_LENGTH] = (char *)malloc(window * sizeof(char) * WORD_LENGTH );
//char (*context_word)[WORD_LENGTH] = ()malloc(sizeof);
        //real p0 = 0.0;
        //p0 = compute_p0(blank_count, blank_index, sentence_length, context_word, word_possibility_p0);
//real p1 = 0.0, p_minus = 0.0;
        compute_p1(iter, blank_count, blank_index, sentence_length,  pWeight, correct_word);
        //compute_p1(iter, blank_count, blank_index, sentence_length, context_word,random_word, possibility_flag, word_possibility_p1, word_possibility_p0,p0, pWeight);

#ifdef _DEBUG_
    /*    int j = 0;
        for(i=0; i<blank_count; i++)
        {
            printf("weight for blank word %lld:%s\n", i, correct_word[i].word);
            for(j=0; j<vocab_size; j++)
            {
                printf("word :%s\t weight:%f\n", vocab[j].word, pWeight[i*vocab_size+j].weight);
            }
        }*/
#endif // _DEBUG_

        for(i=0; i<sentence_length; i++)
            qsort( &pWeight[i*vocab_size], vocab_size, sizeof(pWeight[0]), compare_weight);


#ifdef _DEBUG_
  printf("sorted result for blank word to debug.\n");
  int j =-1;
        for(i=0; i<blank_count; i++)
        {
            printf("%lld blank word:%s\n", i, correct_word[i].word);
            for(j=0; j<vocab_size; j++)
            {
                printf("word :%s\t weight:%f\n", vocab[pWeight[blank_index[i]*vocab_size+j].index].word, log(pWeight[blank_index[i]*vocab_size+j].weight));

            }
        }

#endif // _DEBUG_


     //   print_predicted_words(blank_count, correct_word, blank_index, pWeight, right_count_all);
//    accurcy_rate = (double)right_count/(double)blank_total;
//    printf("total blank:%lld    ", blank_total);
//    printf("number predicted correctly:%lld\n", right_count);
//    printf("the accuracy_rate is :%f\n", accurcy_rate);
/////////////////////////////////////////
//test if the context_index is right.
//   printf("the  current blank index is:%d\n",index);
//    printf("the  current context index is:");
// for(i=0; i<window; i++)
// printf("%d ",context_index[i]);
// printf("\n");
        free_fun(blank_index, correct_word, word_possibility_p0,  word_possibility_p1,pWeight, random_word_index, random_word, possibility_flag);
    }//单个句子循环结束
//printf("blank_rate:%f   test_number:%lld    set_size:%d   epson:%.12f\n",blank_rate, sentence_count, NMAX_POSSIBILITY, epson);
    for(NMAX_POSSIBILITY = 1; NMAX_POSSIBILITY<=SET_SIZE_MAX; NMAX_POSSIBILITY+=1)
    {
        printf("blank_rate:%f   test_number:%lld    set_size:%-4d iter:%f\tlearning rate:%f\t",blank_rate, sentence_count, NMAX_POSSIBILITY, iter, learning_rate);
        accurcy_rate = (double)right_count_all[NMAX_POSSIBILITY-1]/(double)blank_total;
        printf("total blank:%lld\t", blank_total);
        printf("number predicted correctly:%d\t", right_count_all[NMAX_POSSIBILITY-1]);
        printf("the accuracy_rate is :%f\n", accurcy_rate);
    }
    fclose(pTest_file);
  /*  if(context_word != NULL)
    {
        free(context_word);
        context_word = NULL;
    }*/
    return 0;
}

int ArgPos(char *str, int argc, char **argv)
{
    int a;
    for (a = 1; a < argc; a++) if (!strcmp(str, argv[a]))
        {
            if (a == argc - 1)
            {
                printf("Argument missing for %s\n", str);
                exit(1);
            }
            return a;
        }
    return -1;
}

// Reads a single word from a file, assuming space + tab + EOL to be word boundaries
//从文件中读取1个单词
int CountLength(FILE *fin)
{
    int a = 0;
    int count = 0; //save number of words in a line
    char ch;
    long offset = 0;
    ch = fgetc(fin);
    offset++;
    while(1)
    {
        if( ch != '\n')
        {
            if (ch == 13) continue;
            if ((ch == ' ') || (ch == '\t') )
            {
                if (a > 0)
                {
                    count++;
                    a = 0;
                }
                else;
            }
            else
                a++;
            ch = fgetc(fin);
            offset++;
        }
        else
        {
            count++;
            fseek(fin, -offset, SEEK_CUR);
            offset = ftell(fin);
            return count;
        }
    }
}


/***********************************
//产生count个不同的随机数
//每个随机数在1——length-1之间
//随机数存储在array[]中
//随机数不出现在句子前5个单词中
//随机数不出现在句子后2个
************************************/
int generate_random(int array[], int count, int length)
{
    int *flag = calloc(length, sizeof(int));

	if (flag == NULL)
	printf("fail to malloc memory for flag!\n");

    int i =0, blank_index;
    //srand((unsigned)time(NULL));
    while(i < count)
    {
        blank_index = rand() % (length-2) + 1;
        //if(blank_index == 0)
       // {
            if(flag[blank_index] == 0 )
            {
                array[i] = blank_index;
                flag[blank_index] = 1;
                i++;
            }
        //}
       /* else if(blank_index == length - 1 || blank_index == length-2)
       // {
            //if(flag[blank_index] == 0 && flag[blank_index-1] == 0)
            //{
            //    array[i] = blank_index;
            //    flag[blank_index] = 1;
            //    i++;
           // }
         //  continue;
       // }
       // else
       // {
            if(flag[blank_index] == 0 && flag[blank_index-1] == 0 && flag[blank_index+1] == 0 && flag[blank_index-2] == 0 && flag[blank_index+2] == 0)
            {
                array[i] = blank_index;
                flag[blank_index] = 1;
                i++;
            }
        }*/
    }
    if(flag != NULL)
    {
        free(flag);
        flag = NULL;
    }
    return 0;
}

/*
*compute the possibility of blank_word given contex word
*Input:blank_word, context_word
*return:probability
*/
real compute_word_possibility (char blank_word[WORD_LENGTH], char* context_word, int window_size, int vocab_size, int vector_size)
{
    long long j=0;
    long long k =0;
    real probability = 1.0;
    //real *add_vector = NULL;
    //add_vector = (real*)calloc(vector_size, sizeof(real));
    //if(add_vector == NULL)
    //{
      //  printf("fail to allocate memory for add_vector\n");
       // return -1.0;
    //}
    //Add_vector(window_size, vocab_size, vector_size, context_word, add_vector);
    int codelen = 0;
    int point = -1;
    // real inner_product = 0.0;
    double t= 0.0 ;//save exp(inner_product);
    k=0;
    //find index of blank word
    int current_index  = search_word_index(blank_word);
    int context_index = search_word_index(context_word);


   /* while(strcmp(blank_word, vocab[k].word))
    {
        k++;
        if(k >= vocab_size)
        {
            printf("the blank_word is not in vocabulary\n");
            return 1.0;
        }
    }*/
#ifdef _DEBUG_
    //long long object_index = k;
    if(current_index < 0 || current_index >= vocab_size)
    printf("%s's current_index is illegal:%d(in compute_word_possibility)\n", blank_word, current_index);
    if(context_index < 0 || context_index >= vocab_size)
    printf("%s's context_index is illegal:%d(in compute_word_possibility)\n", context_word, context_index);
#endif // _DEBUG_



    codelen = vocab[context_index].codelen;
    for(j=0; j<codelen; j++)
    {
        real inner_product = 0.0;
        point = vocab[context_index].point[j];
        if(vocab[context_index].code[j] == '0') //word2vec 0 is defined to positive///////////////////////////////////////////////////////////////////////////////
        {
            for(k=0; k<vector_size; k++)
            {
                inner_product += vec[current_index * vector_size + k] * weight[point*vector_size + k];
            }
            t = exp(inner_product);
            probability *=  ( t / ( 1 + t) );
        }
        else if(vocab[context_index].code[j] == '1')////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        {
            for(k=0; k<vector_size; k++)
            {
                inner_product += vec[current_index * vector_size + k] * weight[point*vector_size + k];
            }
            t = exp(inner_product);
            probability *=  ( 1 / ( 1 + t) );
        }
    }
    /*if(add_vector != NULL)
    {
        free(add_vector);
        add_vector = NULL;
    }*/
    printf("%s|%s : %f\n", context_word, blank_word, probability);
    return probability;
}

int Add_vector(int window_size, int vocab_size, int vector_size, my_word *word, real *add_vector)
{
    int i,j,k;
    int flag = 0; //to confirm all the context is in vcoablary.
    for(j=0; j<window_size; j++)
        for(i=0; i<vocab_size; i++)
        {
            if( !strcmp(word[j].word, vocab[i].word) )
            {
                flag++;
                for(k=0; k<vector_size; k++)
                    add_vector[k] += vec[i*vector_size + k];
            }
        }
    if(flag < window_size)
    {
        if(flag == 0)
        {
            printf("There is 2 context is not in vocabulary\n");
            return -1;
        }
        if(flag == 1)
        {
            printf("There is 1 context is not in vocabulary\n");
            return -2;
        }
    }
    //       FILE *pf = fopen("test_product.txt","wb");
    //       fprintf(pf,"add_vector:\n");
    for(k=0; k<vector_size; k++)
    {
        add_vector[k] /= window_size;
        // fprintf(pf,"%f,", add_vector[k]);
    }
    return 0;
}


/*
*Input:pointer of sentence
*      point of context word
*      legal blank_index
*Output:context word
*/
int get_context_word(struct test_word* pSentence, int sentence_length, int blank_index, int window_size, my_word *context_word)
{
    //int i = 0;
    if(blank_index <= 0 || blank_index >= sentence_length -1)
    {
        printf("Illegal blank_index.\n");
        return -1;
    }
    //for(i=0; i<window_size; i++)
    strcpy(context_word[0].word, sentence[blank_index-1].word);
    strcpy(context_word[1].word, sentence[blank_index+1].word);
#ifdef _DEBUG_
    /* for(i=0; i<window_size; i++)
    {
    printf("the %d context word is %s\n",i ,word[i].word);
    }*/
#endif // _DEBUG_
    return 0;
}

int read_origin(char *word_file, char *weight_file, char *vector_file)
{
    FILE *pWord_file;
    FILE *pWeight_file;
    FILE *pVector_file;
    pWord_file = fopen(word_file,"rb");
    if (pWord_file  == NULL)
    {
        printf("Word file not found\n");
        return -1;
    }
    pWeight_file = fopen(weight_file,"rb");
    if (pWeight_file  == NULL)
    {
        printf("weight file not found\n");
        return -1;
    }
    fscanf(pWord_file, "%lld", &vocab_size);	//读入单词总数
    fscanf(pWord_file, "%lld", &vector_size);	//读入词向量维度
    //vocab = (struct vocab_word *)calloc(vocab_size, sizeof(struct vocab_word));
    posix_memalign((void **)&vocab, 256, vocab_size * sizeof(struct vocab_word));
    if(vocab == NULL)
    {
        printf("Cannot allocate memory: %lld MB      %lld\n", vocab_size * sizeof(struct vocab_word) / 1048576, vocab_size);
        return -1;
    }
    posix_memalign((void **)&weight, 256, vocab_size * vector_size * sizeof(real));
    //weight = (real *)calloc(vocab_size*vector_size, sizeof(real));
    if(weight == NULL)
    {
        printf("Cannot allocate memory: %lld MB     %lld\n", vocab_size * vector_size * sizeof(real) / 1048576, vocab_size);
        return -1;
    }
    //read weights from pWeight_file
    int i,j;
    for(i=0; i<vocab_size; i++)
        for(j=0; j<vector_size; j++)
            fscanf(pWeight_file,"%lf",&weight[i*vector_size+j]);
    fclose(pWeight_file);
    //write weights to test_weight.txt to test if read is right
    /* pWeight_file = fopen("test_weight.txt","wb");
    //scanf("%f",&weight[0]);
    //printf("%f ",weight[0]);
    for(i=0; i<vocab_size; i++)
    {
    for(j=0; j<vector_size; j++)
    fprintf(pWeight_file ,"%f ", weight[i*vector_size+j]);
    fprintf(pWeight_file, "\n\n");
    }
    fclose(pWeight_file);*/
    for(i=0; i<vocab_size; i++)
    {
        //char word[MAX_STRING];
        //ReadWord(word, pWord_file);
        //vocab[i].word = (char*)malloc((strlen(word)+1)*sizeof(char));
        //strcpy(vocab[i].word, word);
        fscanf(pWord_file,"%s", vocab[i].word);
        fscanf(pWord_file,"%lld",&vocab[i].cn);
        fscanf(pWord_file,"%d",&vocab[i].codelen);
        vocab[i].code = (char*)malloc( (vocab[i].codelen+1) * sizeof(char) );
        //   for(j=0; j<vocab[i].codelen; j++)
        fscanf(pWord_file,"%s", vocab[i].code);
        vocab[i].point = (int*)malloc( vocab[i].codelen * sizeof(int));
        for(j=0; j<vocab[i].codelen; j++)
            fscanf(pWord_file,"%d", &vocab[i].point[j]);
    }
    fclose(pWord_file);
//test if input is right
    /* pWord_file = fopen("test_word.txt","wb");
    for(i=0; i<vocab_size; i++)
    {
    fprintf(pWord_file, "%-15s %-8lld %-4d %-13s",vocab[i].word, vocab[i].cn, vocab[i].codelen, vocab[i].code);
    //  for(j=0; j<vocab[i].codelen; j++)
    //     fprintf(pWord_file, "%s",vocab[i].code);
    //   fprintf(pWord_file,"    ");
    for(j=0; j<vocab[i].codelen; j++)
    fprintf(pWord_file, "%d ",vocab[i].point[j]);
    fprintf(pWord_file,"\n");
    }
    fclose(pWord_file);*/
//read word vector from file
    pVector_file = fopen(vector_file,"rb");
    if (pVector_file  == NULL)
    {
        printf("vector file not found\n");
        return -1;
    }
    fscanf(pWord_file, "%lld", &vocab_size);	//读入单词总数
    fscanf(pWord_file, "%lld", &vector_size);	//读入词向量维度
    vec = (real*)calloc(vocab_size*vector_size, sizeof(real));
    char word[WORD_LENGTH];
    for(i=0; i<vocab_size; i++)
    {
        fscanf(pVector_file,"%s", word);
        for(j=0; j<vector_size; j++)
            fscanf(pVector_file,"%lf", &vec[i*vector_size+j]);
        fscanf(pVector_file,"\n\n");
    }
    fclose(pVector_file);
//test if read word vector is right
    /*   pVector_file = fopen("test_vector.txt","wb");
    for(i=0; i<vocab_size; i++)
    {
    for(j=0; j<vector_size; j++)
    fprintf(pVector_file,"%f ", vec[i*vector_size+j]);
    fprintf(pVector_file,"\n\n");
    }
    fclose(pVector_file);*/
    return 0;
}

//return length of sentence
int read_sentence(FILE *pTest_file)
{
//   char ch = ' ';
    if (pTest_file  == NULL)
    {
        printf("Test file not found\n");
        return -1;
    }
    if(sentence != NULL)
    {
        free(sentence);
        sentence = NULL;
    }
    int sentence_length = 0;
    int i =0;
    //   bool flag=0;
    sentence_length = CountLength(pTest_file);
    sentence = (struct test_word*)malloc( sentence_length * sizeof(struct test_word));
    if(sentence == NULL)
    {
        printf("fail to allocate memory for sentence.\n");
        return -1;
    }
    else
    {
        //    sentence_count++;
        for(i=0; i<sentence_length; i++)
        {
            fscanf(pTest_file, "%s", sentence[i].word);
            sentence[i].flag = true;
        }
        //ch = fgetc(pTest_file);
        fgetc(pTest_file);
        //fscanf(pTest_file, "%s", new_line);
    }
    return sentence_length;
}

int compare_weight( const void *a ,const void *b)
{
    return (*(struct word_weight*)a).weight < (*(struct word_weight*)b).weight ? 1 : -1;
}

/*
*output information about Parameters
*save the Parameters from commandlinde
* */
int commandlinde(int argc, char **argv)
{
    int i = -1;
    if (argc == 1)
    {
        printf("plan predition toolkit v 0.1c\n\n");
        printf("Options:\n");
        printf("Parameters for training:\n");
        printf("\t-word_file <file>\n");
        printf("\t-weigth_file <file>\n");
        printf("\t-vector_file <file>\n");
        printf("\t-test_file <file>\n");
        printf("\t-window <int>\n");
        printf("\t-set_size <int>\n");
        printf("\t-blank_rate <double>\n");
        printf("\t-epson <double>\n");
        printf("\t-iter <int>\n");
        printf("\t-learning_rate <double>\n");
    }
    if ((i = ArgPos((char *)"-word_file", argc, argv)) > 0) strcpy(word_file, argv[i+1]);
    if ((i = ArgPos((char *)"-weight_file", argc, argv)) > 0) strcpy(weight_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-vector_file", argc, argv)) > 0) strcpy(vector_file, argv[i+1]);
    if ((i = ArgPos((char *)"-window", argc, argv)) > 0) window=atoi(argv[i + 1]);
    if ((i = ArgPos((char *)"-test_file", argc, argv)) > 0) strcpy(test_file, argv[i+1]);
    if ((i = ArgPos((char *)"-set_size", argc, argv)) > 0) NMAX_POSSIBILITY=atoi(argv[i + 1]);
    if ((i = ArgPos((char *)"-blank_rate", argc, argv)) > 0) blank_rate=atof(argv[i + 1]);
    //  if ((i = ArgPos((char *)"-epson", argc, argv)) > 0) epson=atof(argv[i + 1]);
    if ((i = ArgPos((char *)"-iter", argc, argv)) > 0) iter=atof(argv[i + 1]);
    if ((i = ArgPos((char *)"-learning_rate", argc, argv)) > 0) learning_rate=atof(argv[i + 1]);
    return 0;
}

int init(int blank_count, int **possibility_flag, my_word **correct_word, my_word **random_word ,int **blank_index, int sentence_length)
{
    if(blank_count > 0)
    {
        *possibility_flag = (int*)calloc(blank_count, sizeof(int));
        * correct_word = (my_word*)malloc(blank_count * sizeof(my_word));
        * random_word = (my_word*)malloc(blank_count*sizeof(my_word));
    }
    if(correct_word == NULL)
    {
        printf("fail to allocate memory for correct_word.\n");
        return -1;
    }
    if(random_word == NULL)
    {
        printf("fail to allocate memory for random_word.\n");
        return -1;
    }
    if(possibility_flag == NULL)
    {
        printf("fail to allocate memory for possibility_flag.\n");
        return -1;
    }
    //blank_count = 1;
    //blank_total++;
    if(blank_count == 0)
    {
        if(sentence != NULL)//sentence is a global pointer initialized by read_sentence()
        {
            free(sentence);
            sentence = NULL;
        }
        printf("blank_count of this sentence is 0.\n");
        return -2;
    }
    //generate the black index randomly.
    if(blank_count > 0)
    {
        *blank_index = (int*)malloc(blank_count*sizeof(int));
        if(blank_index == NULL)
        {
            printf("fail to allocate memory for blank_index or the black count is 0.\n");
            return -1;
        }
        else
            generate_random(*blank_index, blank_count, sentence_length);
    }
    return 0;
}

int free_fun(int *blank_index, my_word *correct_word, real *word_possibility_p0, real *word_possibility_p1,struct word_weight* pWeight, int *random_word_index, my_word *random_word, int *possibility_flag)
{
    if(sentence != NULL)
    {
        //  printf("sentence address:%x\n",sentence);
        free(sentence);
        sentence = NULL;
    }
    if (blank_index != NULL)
    {
        // printf("blank_index address:%x\n",blank_index);
        free(blank_index);
        blank_index = NULL;
    }
    if(correct_word != NULL)
    {
        free(correct_word);
        correct_word = NULL;
    }
    if( word_possibility_p0 != NULL)
    {
        free(word_possibility_p0);
        word_possibility_p0 = NULL;
    }
    if( word_possibility_p1 != NULL)
    {
        free(word_possibility_p1);
        word_possibility_p1 = NULL;
    }
    if(pWeight != NULL)
    {
        free(pWeight);
        pWeight = NULL;
    }
    if(random_word_index != NULL)
    {
        free(random_word_index);
        random_word_index = NULL;
    }
    if(random_word!= NULL)
    {
        free(random_word);
        random_word = NULL;
    }
    if(possibility_flag != NULL)
    {
        free(possibility_flag);
        possibility_flag = NULL;
    }
    return 0;
}

int print_predicted_words(int blank_count, my_word *correct_word, int *blank_index, struct word_weight* pWeight, int *right_count_all)
{
    int i=0, j=0;
    int nmax_word_index = -1;

    //save the right predicted number in
    //        int right_count_all[20];
    //        for(i=0; i<NMAX_POSSIBILITY; i++)
    //            right_count_all[i] = 0;
    for(NMAX_POSSIBILITY = 1; NMAX_POSSIBILITY<=SET_SIZE_MAX; NMAX_POSSIBILITY+=1)
    {
        //           right_count = 0;
        for(i=0; i<blank_count; i++)
        {
#ifdef _DEBUG_
            printf("\ncorrect_word %d:%-25s\n",i, correct_word[i].word);
            printf("predict words for blank %d\n",blank_index[i]);
#endif // _DEBUG_
            for(j=0; j<NMAX_POSSIBILITY; j++)
            {
                nmax_word_index = pWeight[blank_index[i]*vocab_size+j].index;
#ifdef _DEBUG_
                printf("%-45s%-15f\n",vocab[nmax_word_index].word, pWeight[blank_index[i]*vocab_size+j].weight);
#endif // _DEBUG_

                if( !strcmp(correct_word[i].word, vocab[nmax_word_index].word) )
                {
                    right_count_all[NMAX_POSSIBILITY-1]++;
                    //sentence[index].flag = true;
                }
                //right_count_all[NMAX_POSSIBILITY-1]++;
                //sentence[index].flag = true;
            }
        }
    }
    return 0;
}

/*real compute_p0(int blank_count, int *blank_index, int sentence_length, my_word *context_word, real *word_possibility_p0 )
{
    int i = 0,j = 0;
    real p0 = 0.0;
    real temp = 0.0;

    for(i=0; i<blank_count; i++)
    {
#ifdef _DEBUG_
        //    printf("the current blank index: %d\n", blank_index[i]);
#endif // _DEBUG_
        if(blank_index[i] == 0)////////////////////////////////////////////////////////////////////////////////////////////////////////////
        {
            get_context_word( sentence, sentence_length, 1, window, context_word);
            temp = compute_word_possibility ( sentence[1].word, context_word, window, vocab_size, vector_size);
            word_possibility_p0[i] = log(temp);
            p0 += word_possibility_p0[i];
        }
        else if(blank_index[i] == 1)
        {
            get_context_word( sentence, sentence_length, 1, window, context_word);
            temp = compute_word_possibility ( sentence[1].word, context_word, window, vocab_size, vector_size);
            temp = log(temp);
            word_possibility_p0[i] += temp;
            get_context_word( sentence, sentence_length, 2, window, context_word);
            temp = compute_word_possibility ( sentence[2].word, context_word, window, vocab_size, vector_size);
            temp = log(temp);
            word_possibility_p0[i] += temp;
            p0 += word_possibility_p0[i];
        }
        else if(blank_index[i] == sentence_length-1)
        {
            get_context_word( sentence, sentence_length, sentence_length-2, window, context_word);
            temp = compute_word_possibility ( sentence[sentence_length-2].word, context_word, window, vocab_size, vector_size);
            temp = log(temp);
            word_possibility_p0[i] += temp;
            p0 += word_possibility_p0[i];
        }
        else if(blank_index[i] == sentence_length-2)
        {
            get_context_word( sentence, sentence_length, sentence_length-2, window, context_word);
            temp = compute_word_possibility ( sentence[sentence_length-2].word, context_word, window, vocab_size, vector_size);
            temp = log(temp);
            word_possibility_p0[i] += temp;
            get_context_word( sentence, sentence_length, sentence_length-3, window, context_word);
            temp = compute_word_possibility ( sentence[sentence_length-3].word, context_word, window, vocab_size, vector_size);
            temp = log(temp);
            word_possibility_p0[i] += temp;
            p0 += word_possibility_p0[i];
        }
        else
        {
            for(j=0; j<window+1; j++)
            {
                get_context_word( sentence, sentence_length, blank_index[i]-1+j, window, context_word);
                temp = compute_word_possibility ( sentence[blank_index[i]-1+j].word, context_word, window, vocab_size, vector_size);
                temp = log(temp);
                word_possibility_p0[i] += temp;
            }
            p0 += word_possibility_p0[i];
        }
    }
    return p0;
}*/

real compute_p1(double iter, int blank_count, int *blank_index, int sentence_length, struct word_weight* pWeight, my_word* correct_word)
{
    int j = -1;
   // real p1 = 0.0;
   // real temp = 0.0;
   // real p_minus = 0.0;
    //int flag = 0;
    //long long random_index = -1;
    int* random_index = (int*)calloc(blank_count, sizeof(int));
    //char temp_word[WORD_LENGTH]; //save the random word.
    int iter_count = 0;
    real pd = 0.0;

#ifdef _DEBUG_
    real pd_correct[blank_count];
    int  count_big[blank_count];
    for(j=0; j<blank_count; j++)
    {
       pd_correct[j] = -1.0;
        count_big[j] = 0;
    }
#endif // _DEBUG_

    //my_word *temp_sentence_word = (my_word*)malloc(blank_count*sizeof(my_word));

#ifdef _DEBUG_
int i = 0;
real pd_all[vocab_size];
for(i=0;i<vocab_size;i++)
    pd_all[i] = -1.0;
for(i=0; i<blank_count; i++)
{
    for(j=1; j<vocab_size; j++)
    {
          strcpy(sentence[blank_index[i]].word, vocab[j].word);
          pd = compute_partial_derivative(sentence, sentence_length, window, j, pWeight);
          pd_all[j] = pd;
    }
    int obj_index = search_word_index(correct_word[i].word);
    int big_pd_num = 0;
    printf("correct word:%s\n",correct_word[i].word);
    for(j=0;j<vocab_size;j++)
    {
        printf("chosen word:%s\tweight:%f\n",vocab[j].word, pd_all[j]);
        if(pd_all[j] >= pd_all[obj_index])
        {
          big_pd_num++;
          printf("*******************************************\n");

          printf("correct word:%s\tweight:%f\t big_word:%s\tweight:%f\n",correct_word[i].word, pd_all[obj_index], vocab[j].word, pd_all[j]);


        }

    }
    printf("correct word:%s\tweight:%f\tbig_pd_num:%d\n",correct_word[i].word, pd_all[obj_index], big_pd_num);

}


#endif // _DEBUG_



  while( iter_count < iter*vocab_size)
    {
#ifdef _DEBUG_
        //  printf("sentence:%lld   itre:%d\n",sentence_count, iter_count);
#endif // _DEBUG_

        //flag = 0;

        //for(k=0; k<blank_count; k++)
            //word_possibility_p1[k] = 0.0;
        //possibility_flag == 0 means p1 < p0.
        //for(j=0; j<blank_count; j++)
            //possibility_flag[j] = 0;
        pd = 0.0;

        for(j=0; j<blank_count; j++)//control update blank_index word
        {
            //random_index = (rand()%(vocab_size-1)) +1;
            //save blank index word in temp_word
            //strcpy(temp_sentence_word[j].word, sentence[blank_index[j]].word);
            random_index[j] = (rand()%(vocab_size-1)) +1;

            //copy random word to blank_index word to compute p1
            strcpy(sentence[blank_index[j]].word, vocab[random_index[j]].word);

            pd = compute_partial_derivative(sentence, sentence_length, window, j, pWeight);

            int correct_index = search_word_index(correct_word[j].word);

#ifdef _DEBUG_
          if(strcmp(vocab[random_index[j]].word, correct_word[j].word) == 0)
          {
            pd_correct[j] = pd;
            int updated_index = pWeight[ blank_index[j]*vocab_size + random_index[j]].index;

            printf("correct_index to be update:%d\t updated index:%d\trandom_index:%d\n", correct_index, updated_index, random_index[j]);
            printf("before update correct_word's weight is:%f\tcorrext_word:%s\tupdated_word:%s\n",  pWeight[ vocab_size*blank_index[j] + random_index[j]].weight,correct_word[j].word, vocab[updated_index].word);
          }
          else if(pd_correct[j] != -1.0)
          {
            if(pd > pd_correct[j])
                count_big[j]++;
          }

        //  printf("sentence:%lld   itre:%d\n",sentence_count, iter_count);
#endif // _DEBUG_



            pWeight[ vocab_size*blank_index[j] + random_index[j]].weight += learning_rate * pd; //update the weights

#ifdef _DEBUG_
          printf("pd:%f\tpWeight:%f\tblank_word_index:%d\tcorrect_word:%s\trandom_word:%s\trandom_index:%d\n",pd, pWeight[ vocab_size*blank_index[j] + random_index[j]].weight, j, correct_word[j].word,sentence[blank_index[j]].word,random_index[j]);
#endif // _DEBUG_

////////////////////////////////////////////////////////////////////////////
            //project to 0-1
           min_max_norm(pWeight, blank_index[j]);

            if(strcmp(vocab[random_index[j]].word, correct_word[j].word) == 0)
                printf("after update correct_word's weight is:%f\n",  pWeight[ vocab_size*blank_index[j] + random_index[j]].weight);
           else
                printf("correct_word's weight(when random not correct):%f\n",  pWeight[ vocab_size*blank_index[j] + correct_index].weight);



            //save random word in random_word
            //strcpy(random_word[j].word, vocab[random_index].word);
#ifdef _DEBUG_
         /*   int i =-1;
            printf("print the min_max_norm result to debug in compute_p1()\n");

            for(i=0; i<vocab_size; i++)
                printf("weight:%f   index:%lld\n",pWeight[blank_index[j]*vocab_size+i].weight, pWeight[blank_index[j]*vocab_size+i].index);*/
#endif // _DEBUG_

        }
            ++iter_count;

        /*for(j=0; j<blank_count; j++)//control update blank_index word
        {
            pd = compute_partial_derivative(sentence, sentence_length, window, j, pWeight);
            //pd = compute_partial_derivative();
            pWeight[ vocab_size*j + random_index[j]].weight += learning_rate * pd; //update the weights

        }*/
    }
#ifdef _DEBUG_
          //  int i =-1;
         //   printf("print the min_max_norm result to debug in compute_p1()\n");

            for(i=0; i<blank_count; i++)
                printf("index:%d   blank_word:%s\tbig_number:%d\tpd_correct:%f\n",i, correct_word[i].word, count_big[i], pd_correct[i]);
#endif // _DEBUG_

    return 0.0;
}
#ifdef _DEBUG_
        // printf("now compute p1\n");
        //  printf("%d random word is：%s\n",j, sentence[blank_index[j]].word);
        // printf("%d blank index: %d\n",j ,blank_index[j]);
#endif // _DEBUG_
       /* p1 = 0.0;
        for(i=0; i<blank_count; i++)//to control to compute p1
        {
            //word_possibility[k] saves possibility of each blank word,add them equals to p1
            //p1 means possibility of sentence.


            if(blank_index[i] == 0)
            {
                get_context_word( sentence, sentence_length, 1, window, context_word);
                temp = compute_word_possibility ( sentence[1].word, context_word, window, vocab_size, vector_size);
                temp = log(temp);
                word_possibility_p1[i] += temp;
                p1 += word_possibility_p1[i];
            }
            else if(blank_index[i] == 1)
            {
                get_context_word( sentence, sentence_length, 1, window, context_word);
                temp = compute_word_possibility ( sentence[1].word, context_word, window, vocab_size, vector_size);
                temp = log(temp);
                word_possibility_p1[i] += temp;
                get_context_word( sentence, sentence_length, 2, window, context_word);
                temp = compute_word_possibility ( sentence[2].word, context_word, window, vocab_size, vector_size);
                temp = log(temp);
                word_possibility_p1[i] += temp;
                //word_possibility[i] *= temp;
                p1 += word_possibility_p1[i];
            }
            else if(blank_index[i] == sentence_length-1)
            {
                get_context_word( sentence, sentence_length, sentence_length-2, window, context_word);
                temp = compute_word_possibility ( sentence[sentence_length-2].word, context_word, window, vocab_size, vector_size);
                temp = log(temp);
                word_possibility_p1[i] += temp;
                p1 += word_possibility_p1[i];
            }
            else if(blank_index[i] == sentence_length-2)
            {
                get_context_word( sentence, sentence_length, sentence_length-2, window, context_word);
                temp = compute_word_possibility ( sentence[sentence_length-2].word, context_word, window, vocab_size, vector_size);
                temp = log(temp);
                word_possibility_p1[i] += temp;
                get_context_word( sentence, sentence_length, sentence_length-3, window, context_word);
                temp = compute_word_possibility ( sentence[sentence_length-3].word, context_word, window, vocab_size, vector_size);
                temp = log(temp);
                word_possibility_p1[i] += temp;
                p1 += word_possibility_p1[i];
            }
            else
            {
                for(k=0; k<window+1; k++)
                {
                    get_context_word( sentence, sentence_length, blank_index[i]-1+k, window, context_word);
                    temp = compute_word_possibility (sentence[blank_index[i]-1+k].word, context_word, window, vocab_size, vector_size);
                    temp = log(temp);
                    word_possibility_p1[i] += temp;
                }
                p1 += word_possibility_p1[i];
            }

        }//finished computing p1

        for(k=0; k<blank_count; k++)
        {

            p_minus = word_possibility_p1[k] - word_possibility_p0[k];
#ifdef _DEBUG_
//  printf("p0:%f   p1:%f   p_minus:%f\n", p0, p1, p_minus);
#endif // _DEBUG_
            if(p_minus > 0.0)
            {
                pWeight[k*vocab_size + random_index].weight += p_minus * learning_rate;
                //after compute 1 p1, then keep the sentence not changed.
                //to compute next p1
                //strcpy(sentence[blank_index[j]].word, temp_word);
                word_possibility_p0[k] = word_possibility_p1[k];
                possibility_flag[k] = 1;
                flag = 1;
                iter_count++;
            }
            else if(p_minus < 0.0)
            {
                pWeight[k*vocab_size+random_index].weight += p_minus * learning_rate;
                strcpy(sentence[blank_index[k]].word, temp_sentence_word[k].word);
                iter_count++;
            }
        }
        //      iter_count++;
        // }
    }//finished 1 itering*/
    //return 0;
//}

int init_pWeight(struct word_weight* pWeight, int blank_count, int sentence_len)
{
    int i = 0,j = 0;
    // *pWeight = (struct word_weight*)malloc(blank_count*vocab_size * sizeof(struct word_weight));
    if(pWeight == NULL)
    {
        printf("fail to allocate memory for pWeight.\n");
        return -1;
    }
    //real temp_random = 0.0;
    //save initial randoms for weight.
    // srand((unsigned int)time(NULL));
    for(i=0; i<sentence_len; i++)
    {
        if(sentence[i].flag == true)
        {
            int index = search_word_index(sentence[i].word);

            for(j = 0; j<vocab_size; j++)
            {
                if(j == 0)
                {
                    pWeight[i*vocab_size+j].weight = -99999.0;
                    pWeight[i*vocab_size+j].index = j;
                }
                // temp_random = rand()/(real)RAND_MAX - 0.5;
                else if(j == index)
                {
                    pWeight[i*vocab_size+j].weight = 1.0;
                    pWeight[i*vocab_size+j].index = j;
                }
                else
                {
                    pWeight[i*vocab_size+j].weight = 0.0;
                    pWeight[i*vocab_size+j].index = j;
                }
            }

        }
        else
        {
            for(j = 0; j<vocab_size; j++)
            {
                if(j == 0)
                {
                    pWeight[i*vocab_size+j].weight = -99999.0;
                    pWeight[i*vocab_size+j].index = j;
                }
                else
                {
                    pWeight[i*vocab_size+j].weight = 1.0/sentence_len;
                    pWeight[i*vocab_size+j].index = j;
                }
            }

        }

    }
    return 0;
}

real compute_partial_derivative(struct test_word* sentence, int sentence_length, int window, int blank_order, struct word_weight* pWeight)
{
    int i,j,k;
    int point = -1;
    int current_index = -1;
    int context_index = -1;
    int vec_elme_index = -1;
    real t = -1.0;
    real sigma = -1.0;
    real inner_product = -1.0;
    real pd = 0.0;
    real context_weight = 0.0;
    real current_weight = 0.0;
    real x = -1.0;

    for(i=1; i<sentence_length-1; ++i) //control the index not to close end of sentence
    {
        current_index = search_word_index(sentence[i].word);
        for(j=-1; j<=1; j+=2) //j control the window size
        {
            context_index = search_word_index(sentence[i+j].word);
            if( -1 == context_index)
            {
                printf("In compute_partial_derivative(): illegal context_index\n");
                return -1.0;
            }

            current_weight = pWeight[i*vocab_size+current_index].weight;
            context_weight = pWeight[vocab_size*(i+j)+context_index].weight;

            // if(sentence[i].flag == true && sentence[i+j].flag == true)
            // {
            for(k=0; k<vocab[context_index].codelen; ++k)
            {
                inner_product = 0.0;
                point = vocab[context_index].point[k];
                if(vocab[context_index].code[k] == '0') //word2vec 0 is defined to positive///////////////////////////////////////////////////////////////////////////////
                {
                    for(vec_elme_index=0; vec_elme_index<vector_size; vec_elme_index++)
                    {
                        inner_product += vec[vector_size*current_index+vec_elme_index] * weight[point*vector_size + vec_elme_index];
                    }



                    x = 1 * inner_product * context_weight * current_weight;
                    t = exp(x);
                    sigma = (t / (1 + t));
                    //probability *=  ( t / ( 1 + t) );
                    //pd +=  pWeight[vocab_size*blank_order+context_index].weight * inner_product; /**********************/
                    pd += (1 - sigma) * inner_product * context_weight * 1;
                }
                else if(vocab[context_index].code[k] == '1')////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                {
                    for(vec_elme_index=0; vec_elme_index<vector_size; vec_elme_index++)
                    {
                        inner_product += vec[vector_size*current_index+vec_elme_index] * weight[point*vector_size + vec_elme_index];
                    }
                    x = -1 * inner_product * context_weight * current_weight;
                    t = exp(x);
                    sigma = (1 / (1 + t));
                    //t = exp(inner_product);
                    //probability *=  ( 1 / ( 1 + t) );
                    //pd +=  -1 * pWeight[vocab_size*blank_order+context_index].weight * inner_product;
                    pd +=  (1 - sigma) * inner_product * context_weight * -1;
                    //pd += -1 * inner_product;
                }
            }

            //}
        }

    }
    return pd;
}

int search_word_index(char *word)
{
    // char *str = word;
    //printf("****%s\n", str);
    //char* str = word;
    int i = 1;
    //int index = -1;
    for(i=1; i<vocab_size; ++i)
    {
        if ( 0 == strcmp(word, vocab[i].word) )
            return i;
    }
    printf("WARNING:%sis not in vocabulary\n(in search_word_index())", word);
    return -1;

}


int min_max_norm(struct word_weight* pWeight, int sentence_index)
{

int i = 0;
/*#ifdef _DEBUG_
    printf("weight before min_max_norm in min_max_norm:\n");
    for(i=0; i<vocab_size; i++)
        printf("weight:%f\t index:%lld\n", pWeight[sentence_index*vocab_size+i].weight, pWeight[sentence_index*vocab_size+i].index);

#endif // _DEBUG_*/
    real max = pWeight[vocab_size*sentence_index+1].weight;
    real min = pWeight[vocab_size*sentence_index+1].weight;
    real denominator = 0;


    for(i=2; i<vocab_size; i++)
    {
        if(pWeight[vocab_size*sentence_index+i].weight > max)
        {
            max = pWeight[vocab_size*sentence_index+i].weight;
        }
        else if(pWeight[vocab_size*sentence_index+i].weight < min)
        {
            min = pWeight[vocab_size*sentence_index+i].weight;
        }
    }

    denominator = max - min;

    for(i=1; i<vocab_size; i++)
        pWeight[vocab_size*sentence_index+i].weight = ( pWeight[vocab_size*sentence_index+i].weight - min ) / denominator;

/*#ifdef _DEBUG_
    printf("weight after min_max_norm in min_max_norm:\n");
    for(i=0; i<vocab_size; i++)
        printf("weight:%f\t index:%lld\n", pWeight[sentence_index*vocab_size+i].weight, pWeight[sentence_index*vocab_size+i].index);

#endif // _DEBUG_*/

    return 0;
}

