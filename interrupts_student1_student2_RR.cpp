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

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
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

    unsigned int current_time = 0;
    PCB running;
    const unsigned int TIME_QUANTUM = 100; // RR time quantum

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

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
            }
        }

        // edit area for us is between POINT A and POINT B
        // POINT A

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue

        for (int i = 0; i < wait_queue.size(); i++) {
            if (current_time - wait_queue[i].start_time >= wait_queue[i].io_duration) { // if process completed i/o
                wait_queue[i].state = READY; // change process state to READY
                ready_queue.push_back(wait_queue[i]); // move to the back of the ready_queue
                execution_status += print_exec_status(current_time, wait_queue[i].PID, WAITING, READY); // execution status output
                sync_queue(job_list, wait_queue[i]); // job_list management
                wait_queue.erase(wait_queue.begin() + i); // remove process from wait_queue
                i--; // if i/o not finished, we move to the next proces
            }
        }

        // same wait queue management function as shown in the EP .cpp file

        /////////////////////////////////////////////////////////////////

        //////////////////////////EXECUTE RUNNING PROCESS//////////////////////////////
        // execute running process
        if (running.state == RUNNING) {
            unsigned int current_burst_time = current_time - running.start_time;
            running.total_cpu_time += 1; // increment total CPU time for this process
            
            // checking if process is done
            if (running.remaining_time <= current_burst_time) {
                running.state = TERMINATED;
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED); // execution status output
                terminate_process(running, job_list); // helper function to terminate process
                idle_CPU(running); // clear cpu
            }
            
            // if process needs I/O
            // conditions: process has i/o operations and total CPU time is a multiple of io_freq
            else if (running.io_freq > 0 && running.total_cpu_time != 0 && 
                     running.total_cpu_time % running.io_freq == 0) {
                running.remaining_time -= current_burst_time; // subtract actual burst time used
                running.state = WAITING; // process is now waiting
                running.start_time = current_time; // reset start_time for I/O duration tracking
                wait_queue.push_back(running); // add to the back of the wait queue
                execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING); // execution status output
                sync_queue(job_list, running); // job_list management
                idle_CPU(running); // clear cpu
            }
            
            // if time quantum expired
            else if (current_burst_time >= TIME_QUANTUM) {
                running.remaining_time -= TIME_QUANTUM; // subtract quantum from remaining time
                running.state = READY; // preempt process, move back to ready
                ready_queue.push_back(running); // add to back of ready queue (RR logic)
                execution_status += print_exec_status(current_time, running.PID, RUNNING, READY); // execution status output
                sync_queue(job_list, running); // job_list management
                idle_CPU(running); // clear cpu
            }
        }
        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////

        // if CPU idle and ready queue has processes
        if (running.state == NOT_ASSIGNED && !ready_queue.empty()){
            // RR is FIFO
            running = ready_queue.front();
            ready_queue.erase(ready_queue.begin());
            running.start_time = current_time;
            running.state = RUNNING;
            sync_queue(job_list, running);
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING); // execution status output
        }
        /////////////////////////////////////////////////////////////////

        current_time++; // increment current_time within while loop
        // POINT B
    }
    
    //Close the output table
    execution_status += print_exec_footer();

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

    return 0;
}