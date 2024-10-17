#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define Idle 0
#define Busy 1
#define MODLUS 2147483647
#define MULT1 24112
#define MULT2 26143

static long random[100];

double lcg(int mu)
{
    long zi, lowprd, hi31;

    zi = random[mu];
    lowprd = (zi & 65535) * MULT1;
    hi31 = (zi >> 16) * MULT1 + (lowprd >> 16);
    zi = ((lowprd & 65535) - MODLUS) +
         ((hi31 & 32767) << 16) + (hi31 >> 15);
    if (zi < 0)
        zi += MODLUS;
    lowprd = (zi & 65535) * MULT2;
    hi31 = (zi >> 16) * MULT2 + (lowprd >> 16);
    zi = ((lowprd & 65535) - MODLUS) +
         ((hi31 & 32767) << 16) + (hi31 >> 15);
    if (zi < 0)
        zi += MODLUS;
    random[mu] = zi;
    return (zi >> 7 | 1) / 16777216.0;
}

double minimum(double *next_dept_time, int total_server, int *Server_num);

void arrival(double *sys_time, int *total_server, int *num_in_queue, int *num_custs_served, double *next_arr_time, double *q_dwell_time, double *sys_dwell_time,
             double *last_event_time, double *time_interval, double *time_arr, int *server_status, double *next_dept_time, int *mean_interarrival_time, int *mean_service_time, int *index, double *time_arr_sys, int *q_limit);

void departure(double *sys_time, int total_server, int *num_in_queue, int *num_custs_served, double *next_arr_time, double *q_dwell_time, double *sys_dwell_time, int *server_num, double *sys_delay, double *total_sys_delay, double *q_delay, double *total_q_delay,
               double *last_event_time, double *time_interval, double *time_arr, int *server_status, double *next_dept_time, int *mean_interarrival_time, int *mean_service_time, int *index, double *time_arr_sys, int *q_limit);

void move(double *time_arr_sys, int *total_server);

double expon(int mu);

void math_analysis(FILE *fp, int lambda, int mu, int s, int k);

int main()
{
    FILE *fp = fopen("output_MMSK.csv", "w");
    fprintf(fp, "lambda,mu,Wq-sim,W-sim,Lq-sim,L-sim,Wq-math,W-math,Lq-math,L-math\n");
    int testcase;
    printf("Enter the number of testcases:\n");
    scanf("%d", &testcase);
    while (testcase--)
    {
        double sys_time = 0;

        int server_num = 0; // 紀錄有最短離開時間的server
        int num_in_queue = 0;
        int num_custs_served = 0;

        double avg_q_delay = 0;
        double avg_num_in_queue = 0;
        double avg_num_in_system = 0;

        double next_arr_time = 0;
        double q_dwell_time = 0;   // 最後會是所有人在q待的時間加總 (在等候室)
        double sys_dwell_time = 0; // 所有人在system待的時間加總 (進入洗車場到離開)

        double last_event_time = 0; // 上個event發生的時間
        double total_q_delay = 0;
        double q_delay = 0;       //
        double time_interval = 0; // 距離上次到達過了多久

        // metrics

        int mean_interarrival_time = 0; // lambda
        int mean_service_time = 0;      // mu
        int total_server = 0;           // s
        int k;                          // k
        int num_custs_required = 0;     // 請求服務的總人數

        double sys_delay = 0;
        double total_sys_delay = 0;
        double avg_sys_delay = 0;

        int index = -1; // system中編號0~index的server正在使用中

        srand(time(NULL));
        for (int i = 0; i < 100; i++)
            random[i] = rand();

        printf("Enter mean_interarrival_time:\n"); // 輸入lambda
        scanf("%d", &mean_interarrival_time);
        next_arr_time = expon(mean_interarrival_time); // 利用lambda計算出隨機的到達時間

        printf("Enter mean_service_time:\n"); // 輸入mu
        scanf("%d", &mean_service_time);

        printf("Enter total number of server:\n"); // 輸入s
        scanf("%d", &total_server);

        printf("Enter K:\n"); // 輸入k
        scanf("%d", &k);

        printf("Enter num_custs_required:\n"); // 輸入多少人請求系統
        scanf("%d", &num_custs_required);

        int q_limit = k - total_server;          // queue最大長度
        int server_status[total_server + 1];     // 紀錄server 狀態 idle=0 busy=1
        double next_dept_time[total_server + 1]; // 紀錄該server的顧客的離開時間 從index 1開始用
        double time_arr[q_limit + 1];            // queue裡的人的抵達時間 從index 1開始用
        double time_arr_sys[total_server];       // system裡的人的抵達時間 從index 0 開始用

        for (int i = 1; i < q_limit + 1; i++)
            time_arr[i] = 0;
        for (int i = 1; i < total_server + 1; i++)
        {
            time_arr_sys[i - 1] = 0;
            server_status[i] = Idle;
            next_dept_time[i] = pow(10, 30);
        }

        while (num_custs_served < num_custs_required)
        {
            if (next_arr_time > minimum(next_dept_time, total_server, &server_num)) // 下個顧客到達所需時間 > 最快離開的顧客所需時間
            {                                                                       // 那麼下個event就是離開
                departure(&sys_time, total_server, &num_in_queue, &num_custs_served, &next_arr_time, &q_dwell_time, &sys_dwell_time, &server_num, &sys_delay, &total_sys_delay, &q_delay, &total_q_delay,
                          &last_event_time, &time_interval, time_arr, server_status, next_dept_time, &mean_interarrival_time, &mean_service_time, &index, time_arr_sys, &q_limit);
            }
            else
            {
                arrival(&sys_time, &total_server, &num_in_queue, &num_custs_served, &next_arr_time, &q_dwell_time, &sys_dwell_time,
                        &last_event_time, &time_interval, time_arr, server_status, next_dept_time, &mean_interarrival_time, &mean_service_time, &index, time_arr_sys, &q_limit);
            }
        }

        int i = 1;

        while (index != -1) // 所有人都服務過了,但還有人在system中,還沒完成
        {
            sys_time = next_dept_time[i]; // 下個event的時間為 system中最早離開的
            i++;
            sys_delay = sys_time - time_arr_sys[0]; // delay時間為 目前時間-離開的人的抵達時間
            total_sys_delay += sys_delay;
            index--;
        }

        // output result

        printf("\n");
        printf("sys_time = %.8f\n", sys_time);
        printf("num_custs_served = %d\n", num_custs_served);
        printf("num_in_queue = %d\n", num_in_queue);
        printf("total_q_delay = %.8f\n", total_q_delay);

        avg_q_delay = total_q_delay / (double)num_custs_served; // 平均在q裡待多久
        printf("Wq = %.8lf\n", avg_q_delay);

        avg_num_in_queue = q_dwell_time / sys_time; // q的平均長度=每單位時間q有多少人
        printf("Lq = %.8lf\n", avg_num_in_queue);

        avg_sys_delay = total_sys_delay / (double)num_custs_served; // 平均在系統裡待多久
        printf("W = %.8lf\n", avg_sys_delay);

        avg_num_in_system = sys_dwell_time / sys_time; // 系統每單位時間有多少人
        printf("L = %.8lf\n", avg_num_in_system);
        fprintf(fp, "%d,%d,%.8f,%.8f,%.8f,%.8f,", mean_interarrival_time, mean_service_time, avg_q_delay, avg_sys_delay, avg_num_in_queue, avg_num_in_system);
        math_analysis(fp, mean_interarrival_time, mean_service_time, total_server, k);
    }
    fclose(fp);
    return 0;
}

double minimum(double *next_dept_time, int total_server, int *server_num)
{
    double min = next_dept_time[1];
    *server_num = 1;
    for (int i = 2; i < total_server + 1; i++)
    {
        if (min > next_dept_time[i])
        {
            min = next_dept_time[i];
            *server_num = i;
        }
    }
    return min;
}

void arrival(double *sys_time, int *total_server, int *num_in_queue, int *num_custs_served, double *next_arr_time, double *q_dwell_time, double *sys_dwell_time,
             double *last_event_time, double *time_interval, double *time_arr, int *server_status, double *next_dept_time, int *mean_interarrival_time, int *mean_service_time, int *index, double *time_arr_sys, int *q_limit)
{
    *sys_time = *next_arr_time;                                          // 更新時間
    *next_arr_time = *sys_time + (double)expon(*mean_interarrival_time); // 下一次的到達時間 = 目前顧客到達所需時間 + 下一個顧客還要多久才到

    *time_interval = *sys_time - *last_event_time;       // 從上個人到達~這次到達共經過多久時間
    *last_event_time = *sys_time;                        // 設定最後一次顧客到達的時間,用來計算下次的'time_interval'
    *q_dwell_time += (*num_in_queue) * (*time_interval); // q的總等待時間 = queue裡的人數 * 距離上次抵達過了多久

    int server_avail = 0;
    for (int i = 1; i < *total_server + 1; i++)
    {
        if (server_status[i] == Idle)
            server_avail++;
    }

    if (server_avail > 0) // system 有server可用
    {
        *sys_dwell_time += (*num_in_queue) * (*time_interval); // 新來的到queue的屁股,waiting queue裡的人所有等待時間加總到A_U_S
    }
    else
    {
        *sys_dwell_time += (*num_in_queue + 1) * (*time_interval); // 因為server is busy,原本可以被服務的進不去,所以queue裡的數量為原本的+1
    }

    for (int i = 1; i < *total_server + 1; i++) // busy代表還在system中,需要計算存在時間
    {
        if (server_status[i] == Busy)
        {
            *sys_dwell_time += (1) * (*time_interval);
        }
    }

    for (int i = 1; i < *total_server + 1; i++) // 有idle就拿去用
    {
        if (server_status[i] == Idle)
        {
            (*num_custs_served)++;                                             // serve過的人 + 1
            server_status[i] = Busy;                                           // 要使用的server設為忙碌狀態
            next_dept_time[i] = *sys_time + (double)expon(*mean_service_time); // 設定server[i]的完成時間
            (*index)++;                                                        // system中的人 + 1
            time_arr_sys[*index] = *sys_time;                                  // 記錄這個人進到系統中的時間
            return;
        }
    }

    (*num_in_queue)++;            // 上面如果發現有server可以用就會return, 來到這代表無server可用,所以queue的等待顧客數++
    if (*num_in_queue > *q_limit) // 等待人數如果超過queue的長度
    {
        (*num_in_queue)--; // 把剛才加的扣回來,等待人數維持在queue的最大長度
    }
    else
    {
        time_arr[*num_in_queue] = *sys_time; // 丟入q,並紀錄進入queue的人的抵達時間
    }
}
void departure(double *sys_time, int total_server, int *num_in_queue, int *num_custs_served, double *next_arr_time, double *q_dwell_time, double *sys_dwell_time, int *server_num, double *sys_delay, double *total_sys_delay, double *q_delay, double *total_q_delay,
               double *last_event_time, double *time_interval, double *time_arr, int *server_status, double *next_dept_time, int *mean_interarrival_time, int *mean_service_time, int *index, double *time_arr_sys, int *q_limit)
{
    *sys_time = next_dept_time[*server_num];
    *time_interval = *sys_time - *last_event_time;
    *last_event_time = *sys_time;
    *q_dwell_time += (*num_in_queue) * (*time_interval);

    *sys_dwell_time += (*num_in_queue) * (*time_interval); // 離開時不必考慮有無server可用,直接計算waiting queue裡的人所有等待時間加總

    for (int i = 1; i < total_server + 1; i++)
    {
        if (server_status[i] == Busy)
        {
            *sys_dwell_time += (1) * (*time_interval);
        }
    }

    if (*num_in_queue == 0) // q裡沒人
    {
        server_status[*server_num] = Idle;         // 無人使用 設為idle
        next_dept_time[*server_num] = pow(10, 30); // 完成時間設為最大值,避免被選用到
        *sys_delay = *sys_time - time_arr_sys[0];  // 目前時間 - 離開的人的抵達時間
        *total_sys_delay += *sys_delay;
        (*index)--;
        move(time_arr_sys, &total_server);
    }
    else // q裡有人
    {
        (*num_in_queue)--;                        // 從q取出一個人
        *q_delay = *sys_time - time_arr[1];       // q內的等待時間 = 目前時間 - 被取出的人的到達時間
        *sys_delay = *sys_time - time_arr_sys[0]; // sys內的等待時間 = 目前時間 - 離開系統的人的到達時間 (因為有人depart了)
        *total_q_delay += *q_delay;               // 加到total
        *total_sys_delay += *sys_delay;           // 加到total
        (*index)--;

        move(time_arr_sys, &total_server); // 把system arr整理
        (*index)++;
        time_arr_sys[*index] = time_arr[1]; // 紀錄進入系統的人的到達

        (*num_custs_served)++;                                               // 服務過的人數++
        next_dept_time[*server_num] = *sys_time + expon(*mean_service_time); // 產生下次抵達時間

        for (int i = 1; i <= *num_in_queue; i++) // 調整q
        {
            time_arr[i] = time_arr[i + 1];
        }
    }
}

void move(double *time_arr_sys, int *total_server) // 調整sys arr
{
    for (int i = 0; i < *total_server; i++)
    {
        time_arr_sys[i] = time_arr_sys[i + 1];
    }
}

double expon(int mu) // 產生stochastic process
{
    return -mu * logf(lcg(1));
}

// below is math analysis

int landaN(float lambda, int k, int n)
{
    if (n < k)
        return lambda;
    return 0;
}

float muN(float mu, int s, int n)
{
    if (n < s)
        return n * mu;
    return s * mu;
}
//
int factorial(int n)
{
    if ((n == 0) || (n == 1))
        return 1;
    else
        return n * factorial(n - 1);
}
//
double P0(float lambda, float mu, int s, int k)
{
    double sum = 1.0;
    int i;
    for (i = 1; i < s; i++)
        sum += pow((lambda / mu), i) / factorial(i);
    sum += (pow((lambda / mu), s) / factorial(s) * ((1 - pow((lambda / (s * mu)), (k - s + 1))) / (1 - (lambda / (s * mu)))));
    return 1 / sum;
}

double Pn(float lambda, float mu, int s, int k, int n)
{
    if (n < s)
        return ((pow((lambda / mu), n)) / factorial(n)) * P0(lambda, mu, s, k);
    return ((pow((lambda / mu), n)) / (factorial(s) * pow(s, (n - s)))) * P0(lambda, mu, s, k);
}

double Lq(float lambda, float mu, int s, int k)
{
    double result = 0;
    int i;
    for (i = s; i < k + 1; i++)
        result += (i - s) * Pn(lambda, mu, s, k, i);
    return result;
}

double L_(float lambda, float mu, int s, int k)
{
    double sum1 = 0, sum2 = 0;
    int i;
    for (i = 0; i < s; i++)
    {
        double temp = Pn(lambda, mu, s, k, i);
        sum1 += i * temp;
        sum2 += temp;
    }
    sum2 = s * (1 - sum2);
    return (Lq(lambda, mu, s, k) + sum1 + sum2);
}

double lamdaEff(float lambda, float mu, int s, int k)
{
    return lambda * (1 - Pn(lambda, mu, s, k, k));
}

double W_(float lambda, float mu, int s, int k)
{
    return (L_(lambda, mu, s, k) / lamdaEff(lambda, mu, s, k));
}

double Wq(float lambda, float mu, int s, int k)
{
    return (Lq(lambda, mu, s, k) / lamdaEff(lambda, mu, s, k));
}

void math_analysis(FILE *fp, int mean_arrival_time, int mean_service_time, int s, int k)
{
    float lambda, mu;
    lambda = 60.0 / (float)mean_arrival_time;
    mu = 1.0 / ((float)mean_service_time / 60.0);

    fprintf(fp, "%lf,", 60.0 * Wq(lambda, mu, s, k));
    fprintf(fp, "%lf,", 60.0 * W_(lambda, mu, s, k));
    fprintf(fp, "%lf,", Lq(lambda, mu, s, k));
    fprintf(fp, "%lf\n", L_(lambda, mu, s, k));
}
