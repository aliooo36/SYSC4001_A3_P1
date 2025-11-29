/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 * @author Muhammad Ali 101291890
 * @author Gregory Horvat 101303925
 * 
 */

#include<interrupts_student1_student2.hpp>

void FCFS(std::vector<PCB> &ready_queue) { // FCFS queue made by sasi, pass ready_queue as parameter
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

// external priority function
void ExternalPriority(std::vector<PCB> &ready_queue){
    std::sort(
        ready_queue.begin(),
        ready_queue.end(),
        [](const PCB &first, const PCB &second){
            if (first.priority == second.priority){
                return (first.arrival_time > second.arrival_time); // if both processes have the same priority, we just default to FCFS
            }
        return (first.priority < second.priority); // actual EP logic
        }
    );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).
                                    // thank you :)

    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;
    std::string memory_status; // separate string for memory output

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                // output requirement similar to assignment 1, thanks sasi
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue

        for (int i = 0; i < wait_queue.size(); i++) {
            if (current_time - wait_queue[i].start_time >= wait_queue[i].io_duration) { // if process completed i/o
                wait_queue[i].state = READY; // change process state to READY
                ready_queue.push_back(wait_queue[i]); // move to the back of the ready_queue
                execution_status += print_exec_status(current_time, wait_queue[i].PID, WAITING, READY); // execution status output
                sync_queue(job_list, wait_queue[i]); // job_list management
                wait_queue.erase(wait_queue.begin() + i); // remove process from wait_queue
                i--; // if i/o not finished, we move to the next process
            }
        }
        /////////////////////////////////////////////////////////////////

        //////////////////////////EXECUTE RUNNING PROCESS//////////////////////////////
        // execute running process
        if (running.state == RUNNING) {
            unsigned int current_burst_time = current_time - running.start_time;
            
            // checking if process is done
            if (running.remaining_time <= current_burst_time) {
                running.state = TERMINATED; // change process status to TERMINATED
                running.completion_time = current_time; // Record completion time
                sync_queue(job_list, running); // Update job_list with completion time
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED); // execution status output
                terminate_process(running, job_list); // helper function to terminate process
                idle_CPU(running); // clear cpu
            }
            
            // if process needs I/O
            // conditions: process has i/o operations and the i/o operation can be performed with the current burst time
            else if (running.io_freq > 0 && current_burst_time >= running.io_freq) {
                running.remaining_time -= running.io_freq;
                running.state = WAITING; // process is now waiting
                running.start_time = current_time; // reset start_time for I/O duration tracking
                wait_queue.push_back(running); // add to the back of the wait queue
                execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING); // execution status output
                sync_queue(job_list, running); // job_list management
                idle_CPU(running); // clear cpu
            }
        }
        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        ExternalPriority(ready_queue); // call EP function we made

        // if CPU idle and ready queue has processes
        if (running.state == NOT_ASSIGNED && !ready_queue.empty()){
            run_process(running, job_list, ready_queue, current_time); // call helper function to run process
            
            // Track first run time for response time calculation
            if (running.first_run_time == -1) { 
                running.first_run_time = current_time;
            }
            
            sync_queue(job_list, running); // ensure job_list is updated
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING); // execution status output
            memory_status += print_memory_status(job_list); // capture memory status whenever a process starts
        }
        /////////////////////////////////////////////////////////////////
        
        for (auto &process : ready_queue) { // increment waiting time for processes in ready_queue
            process.total_waiting_time++;
            sync_queue(job_list, process); // ensure job_list is updated
        }
        
        current_time++; // increment current_time within while loop

    }
    
    //Close the output table
    execution_status += print_exec_footer();
    
    // Calculate performance metrics
    unsigned int total_turnaround = 0; // init new metric variables
    unsigned int total_wait = 0;
    unsigned int total_response = 0;
    unsigned int completed_processes = 0;
    
    for(const auto &process : job_list) { // goes through all the processes within the job_list
        if(process.state == TERMINATED) {
            completed_processes++; // append +1 to completed_processes
            total_turnaround += (process.completion_time - process.arrival_time); //turnaround time calculation
            total_wait += process.total_waiting_time; // total process wait time
            if(process.first_run_time != -1) { // if process ran
                total_response += (process.first_run_time - process.arrival_time); //response time calculation
            }
        }
    }
    
    // Calculate averages (if completed_process is above 0)
    float avg_turnaround = (completed_processes > 0) ? (float)total_turnaround / completed_processes : 0;
    float avg_wait = (completed_processes > 0) ? (float)total_wait / completed_processes : 0;
    float avg_response = (completed_processes > 0) ? (float)total_response / completed_processes : 0;
    float throughput = (current_time > 0) ? (float)completed_processes / current_time : 0;
    
    // add info to execution.txt output
    execution_status += "\n\nPerformance Metrics\n";
    execution_status += "Throughput: " + std::to_string(throughput) + " processes/ms\n";
    execution_status += "Average Turnaround Time: " + std::to_string(avg_turnaround) + " ms\n";
    execution_status += "Average Wait Time: " + std::to_string(avg_wait) + " ms\n";
    execution_status += "Average Response Time: " + std::to_string(avg_response) + " ms\n";
    
    // write memory status to separate file
    write_output(memory_status, "memory.txt");

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;

    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");
    // memory.txt is written inside run_simulation

    return 0;
}