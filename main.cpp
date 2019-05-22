#include <iostream>
#include <fstream>
#include <string>
#include <locale>
#include <queue>
#include <vector>
#include <cstdlib>
#include <random>

using namespace std;

//class that will store the configuration from config.txt
class Configuration
{
    public:
    double seed, init_time, fin_time, arrive_min, arrive_max, quit_prob,
        cpu_min, cpu_max, disk1_min, disk1_max, disk2_min, disk2_max; // all of the values that need to be read from the file
    void read(); //function that will read values from a configuration file (config.txt)
};

//class that will contain data on jobs that arrive
class Job
{
    public:
    int job_number,job_time;
    string job_status;
    string job_type;
    //constructor for first job to be put in event queue
    Job()
    {
        job_number = 1;
        job_time = 0;
        job_status = "arrives";
        job_type = "CPU";
    }

    Job(int number, int time, string status, string type)
    {
        job_number = number;
        job_time = time;
        job_status = status;
        job_type = type;
    }
};
//class to create CPU instance
class CPU
{
    public:
    int status; // Status will be represented by a number with 0 being idle and 1 being busy
    Job job; //Job that is currently in the cpu, if any

    CPU()
    {
        status = 0;
    }
};
//class to create Disk instance
class Disk
{
    public:
    int status; // Status will be represented by a number with 0 being idle and 1 being busy
    Job job; //Job that is currently in disk1, if any

    Disk()
    {
        status = 0;
    }
};

//A struct for the priority queue that makes sure jobs with the lowest time are at the highest priority
struct LessThanByTime
{
  bool operator()(const Job& lhs, const Job& rhs) const
  {
    return lhs.job_time > rhs.job_time;
  }
};

//read values needed from the configuration file
void Configuration::read()
{
    ifstream config;
    string line;
    int i = 0;
    double values[12];
    config.open("config.txt");
    if (config){
        while(getline(config,line))
        {
            if(isdigit(line[0]))
            {
                double value = std::stod (line); //converts the number from the file into a double
                values[i] = value;
                i++;
            }
        }
        config.close();

        seed = values[0];
        init_time = values[1];
        fin_time = values[2];
        arrive_min = values[3];
        arrive_max = values[4];
        quit_prob = values[5];
        cpu_min = values[6];
        cpu_max = values[7];
        disk1_min = values[8];
        disk1_max = values[9];
        disk2_min = values[10];
        disk2_max = values[11];
    }
    else cout << "Unable to open file";

}

int main()
{
    Configuration configuration;
    configuration.read();

    //This block is used to make random numbers in a uniform manner, where it is spread out among the interval
    random_device rd;
    mt19937 generator(rd());
    //create random number generator for arrival time
    uniform_int_distribution<int> arrival_time(configuration.arrive_min,configuration.arrive_max);
    //create random number generator for time spent in CPU
    uniform_int_distribution<int> cpu_time(configuration.cpu_min,configuration.cpu_max);
    //create random number generator for time spent in Disk1
    uniform_int_distribution<int> disk1_time(configuration.disk1_min,configuration.disk1_max);
    //create random number generator for time spent in Disk2
    uniform_int_distribution<int> disk2_time(configuration.disk2_min,configuration.disk2_max);
    //create random number generator to determine if a job at the CPU will quit
    uniform_real_distribution<double> cpu_quit(0,1);
    //create a random number to select which disk queue to enter if both disk queues are the same size
    uniform_int_distribution<int> disk_select(1,2);


    //create FIFO queues for CPU, Disk1, Disk2, and a priority queue to store all events in chronological orders
    queue <Job> CPUq;
    queue <Job> Disk1q;
    queue <Job> Disk2q;
    priority_queue <Job, vector<Job>, LessThanByTime> Events;

    //initialize the cpu, disk1, and disk2, set all to idle
    CPU cpu = CPU();
    Disk disk1 = Disk(),disk2 = Disk();

    //push the end condition and the first job to be processed
    Events.push(Job(0,configuration.fin_time,"finished simulation","end")); //final event to be popped from the event queue
    Events.push(Job());

    int x = 2;//x will be used when constructing new jobs by assigning a job number
    int i = 0, j = 0, k = 0, l = 0, m = 0, n = 0; // variables to track iterations throughout simluation
    int current_time = configuration.init_time; //start at 0 and then update as the simulation runs
    int busy_time = 0, arrive_time = 0; //Time that is spent on a job arriving and being processed in a component
    double quit, disk_rand;
    double cpu_utilization, disk1_utilization, disk2_utilization; //utilization of each component
    double cpu_throughput, disk1_throughput, disk2_throughput; //throughput of each component

    int cpu_jobs_completed, disk1_jobs_completed, disk2_jobs_completed; //tracks the number of jobs completed at each component, used to calculate throughput
    int time_busy_cpu, time_busy_disk1, time_busy_disk2; // total time that each component is busy, used to calculate utilization
    int cpu_busy, disk1_busy, disk2_busy; //time that each event is busy.
    int cpu_max_busy, disk1_max_busy, disk2_max_busy; //maximum response time

    int cpuq_size, disk1q_size, disk2q_size; //used to calculate the average queue size
    int cpuq_max_size, disk1q_max_size, disk2q_max_size; //maximum queue size

    ofstream log;
    log.open(("log.txt"));

    log << "Seed: " << configuration.seed << '\n';
    log << "Initial Time: " << configuration.init_time << '\n';
    log << "Final time: " << configuration.fin_time << '\n';
    log << "Arrive Min: " << configuration.arrive_min << '\n';
    log << "Arrive Max: " << configuration.arrive_max << '\n';
    log << "Quit Prob: " << configuration.quit_prob << '\n';
    log << "CPU Min: " << configuration.cpu_min << '\n';
    log << "CPU Max: " << configuration.cpu_max << '\n';
    log << "Disk1 Min: " << configuration.disk1_min << '\n';
    log << "Disk1 Max: " << configuration.disk1_max << '\n';
    log << "Disk2 Min: " << configuration.disk2_min << '\n';
    log << "Disk2 Max: " << configuration.disk2_max << '\n';

    //Simulation loop, stop creating new jobs when last job is created at final time (fin_time)
    while(!Events.empty())
    {
        //log the current event in log.txt
        log << "Job" << Events.top().job_number << " " << Events.top().job_status << " " << Events.top().job_type << " at time unit " << Events.top().job_time << '\n' << '\n';
        current_time = Events.top().job_time;
        //determine the type of job
        if(Events.top().job_type == "CPU")
        {
            //Determine whether event arrives or finishes
            if(Events.top().job_status == "arrives")
            {
                //Create a new job that will arrive in the CPU queue at a later time and insert that event to the Event queue
                arrive_time = current_time + arrival_time(generator);
                Job new_job = Job(x, arrive_time, "arrives", "CPU");
                Events.push(new_job);
                x++;

                //Send the current job to the CPU queue
                CPUq.push(Events.top());

                //if the size of the queue now is at its largest, change the max queue size
                if(cpuq_max_size < CPUq.size())
                {
                    cpuq_max_size = CPUq.size();
                }

                cpuq_size += CPUq.size();
                i++;

                //Check if CPU is idle.
                //If the CPU is idle, process job in front of the queue, set CPU to busy
                //If busy, do nothing
                if(cpu.status == 0)
                {
                    cpu.job = CPUq.front();
                    CPUq.pop();
                    cpuq_size += CPUq.size();
                    i++;

                    cpu_busy = cpu_time(generator);

                    //if the current response time is larger than the current max response time, update
                    if(cpu_max_busy < cpu_busy)
                    {
                        cpu_max_busy = cpu_busy;
                    }

                    busy_time = current_time + cpu_busy;
                    time_busy_cpu += cpu_busy;
                    l++;

                    Job cpu_finish_event = Job(cpu.job.job_number, busy_time, "finishes", "CPU");
                    Events.push(cpu_finish_event);
                    cpu.status = 1; // busy
                }

                Events.pop();
            }

            if(Events.top().job_status == "finishes")
            {
                //Set CPU status to idle (0) and determine if the job leaves the simulation or not
                cpu.status = 0;
                quit = cpu_quit(generator);

                //If quit is less than the quit probability in config.txt, job exits the system at that time
                if(quit < configuration.quit_prob)
                {
                    Job exit = Job(Events.top().job_number, current_time , "exits", "CPU");

                    cpu_jobs_completed++;

                    Events.push(exit);
                }

                //If not, send the current job to the disk with the smallest queue
                else
                {
                   //determine which disk is smaller
                   if(Disk1q.size() < Disk2q.size())
                   {
                    //Generate Disk event for disk1
                    Job disk1_event = Job(Events.top().job_number, current_time, "arrives", "Disk1");
                    Events.push(disk1_event);
                    cpu_jobs_completed++;
                   }

                   else if(Disk1q.size() > Disk2q.size())
                   {
                    //Generate Disk event for disk2
                    Job disk2_event = Job(Events.top().job_number, current_time, "arrives", "Disk2");
                    Events.push(disk2_event);
                    cpu_jobs_completed++;
                   }

                   //If both disks are the same size, Pick a random disk for the job to be put in
                   else
                   {
                        disk_rand = disk_select(generator);

                        if(disk_rand == 1)
                        {
                            //Generate Disk event for disk1
                            Job disk1_event = Job(Events.top().job_number, current_time, "arrives", "Disk1");
                            Events.push(disk1_event);
                            cpu_jobs_completed++;
                        }

                        else if(disk_rand == 2)
                        {
                            //Generate Disk event for disk2
                            Job disk2_event = Job(Events.top().job_number, current_time, "arrives", "Disk2");
                            Events.push(disk2_event);
                            cpu_jobs_completed++;
                        }
                   }
                }

                Events.pop();
            }

            if(Events.top().job_status == "exits")
            {
                //Pop off job from Events, never to be seen again
                Events.pop();
            }
        }

        else if(Events.top().job_type == "Disk1")
        {
            //Determine whether event arrives or finishes
            if(Events.top().job_status == "arrives")
            {
                Disk1q.push(Events.top());

                //if the size of the queue now is at its largest, change the max queue size
                if(disk1q_max_size < Disk1q.size())
                {
                    disk1q_max_size = Disk1q.size();
                }

                disk1q_size += Disk1q.size();
                j++;

                //Check if disk is idle.
                //If the disk is idle, process job in front of the queue, set disk to busy
                //If busy, do nothing
                if(disk1.status == 0)
                {
                    disk1.job = Disk1q.front();
                    Disk1q.pop();
                    disk1q_size += Disk1q.size();
                    j++;

                    disk1_busy = disk1_time(generator);

                    //if the current response time is larger than the current max response time, update
                    if(disk1_max_busy < disk1_busy)
                    {
                        disk1_max_busy = disk1_busy;
                    }

                    busy_time = current_time + disk1_busy;
                    time_busy_disk1 += disk1_busy;
                    m++;

                    Job disk1_finish_event = Job(disk1.job.job_number, busy_time, "finishes", "Disk1");
                    Events.push(disk1_finish_event);
                    disk1.status = 1; //busy
                }
                Events.pop();
            }

            if(Events.top().job_status == "finishes")
            {
                disk1.status = 0;
                Job arrival_from_disk1 = Job(Events.top().job_number, current_time, "arrives", "CPU");

                disk1_jobs_completed++;

                Events.push(arrival_from_disk1);
                Events.pop();
            }
        }

        else if(Events.top().job_type == "Disk2")
        {
            //Determine whether event arrives or finishes
            if(Events.top().job_status == "arrives")
            {
                //push current event into disk2 queue
                Disk2q.push(Events.top());

                //if the size of the queue now is at its largest, change the max queue size
                if(disk2q_max_size < Disk2q.size())
                {
                    disk2q_max_size = Disk2q.size();
                }

                disk2q_size += Disk2q.size();
                k++;

                //Check if disk is idle.
                //If the disk is idle, process job in front of the queue, set disk to busy
                //If busy, do nothing
                if(disk2.status == 0)
                {
                    disk2.job = Disk2q.front();
                    Disk2q.pop();
                    disk2q_size += Disk2q.size();
                    k++;

                    disk2_busy = disk2_time(generator);

                    //if the current response time is larger than the current max response time, update
                    if(disk2_max_busy < disk2_busy)
                    {
                        disk2_max_busy = disk2_busy;
                    }

                    busy_time = current_time + disk2_busy;
                    time_busy_disk2 += disk2_busy;
                    n++;

                    Job disk2_finish_event = Job(disk2.job.job_number, busy_time, "finishes", "Disk2");
                    Events.push(disk2_finish_event);
                    disk2.status = 1; //busy
                }
                Events.pop();
            }

            if(Events.top().job_status == "finishes")
            {
                //set disk2 status to idle, send the job to the cpu and push an arrival event
                disk2.status = 0;
                Job arrival_from_disk2 = Job(Events.top().job_number, current_time, "arrives", "CPU");

                //job completed to be used to calculate throughput
                disk2_jobs_completed++;

                Events.push(arrival_from_disk2);
                Events.pop();
            }
        }
        //if the simulation ends, pop off last event and break simulation loop
        else if(Events.top().job_type == "end")
        {
            Events.pop();
            break;
        }

    }

    log.close();

    //calculate the utilization
    cpu_utilization = time_busy_cpu / (configuration.fin_time - configuration.init_time);
    disk1_utilization = time_busy_disk1 / (configuration.fin_time - configuration.init_time);
    disk2_utilization = time_busy_disk2 / (configuration.fin_time - configuration.init_time);

    //calculate the throughput
    cpu_throughput = cpu_jobs_completed / (configuration.fin_time - configuration.init_time);
    disk1_throughput = disk1_jobs_completed / (configuration.fin_time - configuration.init_time);
    disk2_throughput = disk2_jobs_completed / (configuration.fin_time - configuration.init_time);

    //print the utilization of each component
    cout << "CPU Utilization: " << cpu_utilization * 100<< "%" << '\n';
    cout << "Disk1 Utilization: " << disk1_utilization * 100 << "%" << '\n';
    cout << "Disk2 Utilization: " << disk2_utilization * 100 << "%" << '\n' << '\n';

    //print the throughput of each component
    cout << "Number of Jobs completed per unit of time (CPU): " << cpu_throughput << '\n';
    cout << "Number of Jobs completed per unit of time (Disk1): " << disk1_throughput << '\n';
    cout << "Number of Jobs completed per unit of time (Disk2): " << disk2_throughput << '\n' << '\n';

    //print the average size of each server queue
    cout << "Average size of CPUQ: " << cpuq_size / i << '\n';
    cout << "Average size of Disk1Q: " << disk1q_size / j << '\n';
    cout << "Average size of Disk2Q: " << disk2q_size / k << '\n' << '\n';

    //print the max size of each server queue
    cout << "Max size of CPUQ: " << cpuq_max_size << '\n';
    cout << "Max size of Disk1Q: " << disk1q_max_size << '\n';
    cout << "Max size of Disk2Q: " << disk2q_max_size << '\n' << '\n';

    //print the average response time of each component
    cout << "Average Response Time of CPUQ: " << time_busy_cpu / l << '\n';
    cout << "Average Response Time of Disk1Q: " << time_busy_disk1 / m << '\n';
    cout << "Average Response Time of Disk2Q: " << time_busy_disk2 / n << '\n' << '\n';

    //print the max response time of each component
    cout << "Max Response Time of CPUQ: " << cpu_max_busy << '\n';
    cout << "Max Response Time of Disk1Q: " << disk1_max_busy << '\n';
    cout << "Max Response Time of Disk2Q: " << disk2_max_busy << '\n' << '\n';

    cout << "Open log.txt" << '\n';
    return 0;

}

