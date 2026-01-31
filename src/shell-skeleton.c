#include <errno.h>
#include <stdbool.h> 
#include <stdio.h> // printf(), 
//#fgets() 
#include <string.h>
#include <stdlib.h> // free() include <string.h> // strtok(), 
//#strcmp(), strdup() 
#include <sys/wait.h> // waitpid() include 
#include <termios.h> // termios, TCSANOW, ECHO, ICANON include <unistd.h> // 
//#POSIX API: fork()
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

static char *history[200];
static int history_count = 0;
static int history_pos = 0;



const char *sysname = "ˢₜᵃₛʰ"; 

// TODO : Fill below
const int groupSize = 1; // TODO: change to 2 if you are two people
const char *student1Name = "Can Kadri Eltepe";
const char *student1Id = "0083855";
const char *student2Name = " ***FILL HERE*** ";
const char *student2Id = " ***FILL HERE*** ";



typedef struct cmd_t {
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;  //  how many arguments are there
	char **args;  // pointer to char pointers for each argument
	char *redirects[3]; // stdin/stdout to/from file
	struct cmd_t *next; // for piping
	char *rawline;
} cmd_t;



// Release allocated memory for a command structure
void free_command(cmd_t *cmd) {
    // if arg count is greater than 0 free command args
	if (cmd->arg_count) {
		for (int i = 0; i < cmd->arg_count; ++i)
			free(cmd->args[i]);
		free(cmd->args);
	}

    // free if redirects are not null
	for (int i = 0; i < 3; ++i) {
		if (cmd->redirects[i])
			free(cmd->redirects[i]);
	}

    // free if next command in pipe chain is not null
	if (cmd->next) {
		free_command(cmd->next);
		cmd->next = NULL;
	}
	 if (cmd->rawline) free(cmd->rawline);

    // free the char array for name
	free(cmd->name);
	free(cmd);
}



// Show the command prompt
void show_prompt() {
	char cwd[512], hostname[512];
	gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("👤%s 💻%s 📂%s %s ╰┈➤ ", getenv("USER"), hostname, cwd, sysname);
}

//For handling issues in pipes regarding the empty spaces or the lack of it
static void normalize_pipes(char *buf, size_t cap){
    char out[512];
    size_t j = 0;
    bool in_single = false, in_double = false;

    for(size_t i =0; buf[i] != '\0';i++){
        char c= buf[i];

        if(c== '\'' && !in_double) in_single = !in_single;
        else if (c == '"' && !in_single) in_double = !in_double;

        if(c == '|' && !in_single && !in_double){
            if(j + 3 >= sizeof(out)) break;

            if(j >0 && out[j - 1] != ' ') out[j++] = ' ';
            out[j++] = '|';
            if (buf[i + 1] != '\0' && buf[i + 1] != ' ')
                out[j++] = ' ';
    }else{
            if(j + 1 >= sizeof(out)) break;
            out[j++] =c;
        }
    }
    out[j] = '\0';

    strncpy(buf, out, cap - 1);
    buf[cap - 1] = '\0';
}


// Parse a command string into a command struct
void parse_command(char *buf, cmd_t *cmd) {
	const char *splitters = " \t"; // split at space and tab
	int index, len;
	len = strlen(buf);

	// trim left whitespace
	while (len > 0 && strchr(splitters, buf[0]) != NULL) {
		buf++;
		len--;
	}

    // trim right whitespace
	while (len > 0 && strchr(splitters, buf[len - 1]) != NULL)
        buf[--len] = 0;
	

	// marked for auto-complete
	if (len > 0 && buf[len - 1] == '?') cmd->auto_complete = true;	

	// background execution
	if (len > 0 && buf[len - 1] == '&')	cmd->background = true;	


	char *pch = strtok(buf, splitters);
	if (pch == NULL) {
		cmd->name = (char *)malloc(1);
		cmd->name[0] = 0;
	} else {
		cmd->name = (char *)malloc(strlen(pch) + 1);
		strcpy(cmd->name, pch);
	}

	cmd->args = (char **)malloc(sizeof(char *));

	int redirect_index;
	int arg_index = 0;
	char temp_buf[1024], *arg;

	while (1) {
		// tokenize input on splitters
		pch = strtok(NULL, splitters);
		if (!pch) break;

		arg = temp_buf;
		strcpy(arg, pch);
		len = strlen(arg);

		// trim left whitespace
		while (len > 0 && strchr(splitters, arg[0]) != NULL) {
			arg++;
			len--;
		}

		// trim right whitespace
		while (len > 0 && strchr(splitters, arg[len - 1]) != NULL) 
			arg[--len] = 0;
		

		// empty arg, go for next
		if (len == 0) continue;
		

		// mark for piping to another command
		if (strcmp(arg, "|") == 0) {
            cmd_t *c = malloc(sizeof( cmd_t));
		memset(c,0 , sizeof( cmd_t));
			int l = strlen(pch);
			pch[l] = splitters[0]; // restore strtok termination
			index = 1;
			while (pch[index] == ' ' || pch[index] == '\t')
				index++; // skip whitespaces

			parse_command(pch + index, c);
			pch[l] = 0; // put back strtok termination
			cmd->next = c;
			continue;
		}

		// background process; already marked in cmd
		if (strcmp(arg, "&") == 0) continue;
		

		// mark IO redirection
		redirect_index = -1;
		if (arg[0] == '<') {
			redirect_index = 0; // 0 -> take input from file <
		}

		if (arg[0] == '>') {
			if (len > 1 && arg[1] == '>') {
				redirect_index = 2; // 2 -> append output to file >>
				arg++;
				len--;
			} else {
				redirect_index = 1; // 1 -> write output to file >
			}
		}

		if (redirect_index != -1) {
			cmd->redirects[redirect_index] = malloc(len);
			strcpy(cmd->redirects[redirect_index], arg + 1);
			continue;
		}

		// normal arguments
		if (len > 2 &&
			((arg[0] == '"' && arg[len - 1] == '"') ||
			 (arg[0] == '\'' && arg[len - 1] == '\''))) // quote wrapped arg
		{
			arg[--len] = 0;
			arg++;
		}

		cmd->args = (char **)realloc(cmd->args, sizeof(char *) * (arg_index + 1));

		cmd->args[arg_index] = (char *)malloc(len + 1);
		strcpy(cmd->args[arg_index++], arg);
	}
	cmd->arg_count = arg_index;

    // first argument should be the name of the executable
    // last argument should be NULL to delimit the end

	// increase args size by 2
	cmd->args = (char **)realloc(cmd->args, sizeof(char *) * (cmd->arg_count += 2));

	// shift everything forward by 1
	for (int i = cmd->arg_count - 2; i > 0; --i)
		cmd->args[i] = cmd->args[i - 1];	

	// set args[0] as a copy of name
	cmd->args[0] = strdup(cmd->name);

	// set args[arg_count-1] (last) to NULL
	cmd->args[cmd->arg_count - 1] = NULL;
}
static bool starts_with(const char *s, const char *prefix) {
        return strncmp(s, prefix, strlen(prefix))==0;
}
//we are trying to see if the file is executable with this function
static bool is_executable(const char *path){
	struct stat st;
	if(stat(path, &st)!=0) return false;
	if(!S_ISREG(st.st_mode)) return false;
	return access(path, X_OK) ==0;
}

//returns 1 if the name exists as executable in PATH else 0
static bool command_exact(const char *name) {
         if (!name || !*name) return false;

	 //If it contains / treat as path
	
         if (strchr(name, '/')) return is_executable(name);
         const char *path_env = getenv("PATH");

         if (!path_env) return false;
         char *copy = strdup(path_env);
            if (!copy) return false;

            char *saveptr = NULL;
            for (char *dir = strtok_r(copy, ":", &saveptr); dir; dir = strtok_r(NULL, ":", &saveptr)) {
                char candidate[512];
                snprintf(candidate, sizeof(candidate), "%s/%s", dir, name);
           if (is_executable(candidate))  {
                 free(copy);
            return true;
        }
    }

    free(copy);

    return false ;
}

//for listing the current directory

static void list_curr_dir(void){
        DIR *d = opendir(".");
         if (!d) {
                printf("cant open current directory\n");
                return;
    }
    printf("\n");
        struct dirent *ent;
   while ((ent = readdir(d)) != NULL){
        if (strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0)  continue;
                printf("%s  ", ent->d_name);
    }
    printf("\n");

    closedir(d);
}

//collect matches of prefix from Path executables
// return count and fills out with strdup() names

static int find_matches(const char *prefix, char **out, int out_cap) {
            const char *path_env = getenv("PATH");
            if (!path_env) return 0;

            char *copy = strdup(path_env);
            if (!copy) return 0;
 
            int count = 0;
            char *saveptr = NULL;

            for (char *dir = strtok_r(copy, ":", &saveptr); dir; dir = strtok_r(NULL, ":", &saveptr)) {
                DIR *d = opendir(dir);
                if (!d) continue;

                struct dirent *ent;
                while ((ent = readdir(d)) != NULL) {
                    const char *name = ent->d_name;
                    if (!starts_with(name, prefix)) continue;

            char full[PATH_MAX];
            snprintf(full, sizeof(full), "%s/%s", dir, name);

             if (!is_executable(full)) continue;
//avoid duplicates across the PATH dirs
            bool dup = false;
            for (int i = 0; i < count; i++) {
                if (strcmp(out[i], name) == 0) { dup = true; break; }
                  }
            if (dup) continue;

            if (count < out_cap) out[count++] = strdup(name);
            else break;
      }

        closedir(d);
        if (count >= out_cap) break;
}

    free(copy);
    return count;
}

//for showing the prompt and  the user command written thus far or after the tab 
static void redraw_prompt_and_buf(const char *buf, size_t index){
    show_prompt();
    if(index > 0){
        fwrite(buf, 1, index, stdout);
    }
    fflush(stdout);
}
//I do  it  here because doing it inside the normal process_command gave me problems with the  filling out clear match  part 
static void do_autocomplete_in_prompt(char buf[512], size_t *index_ptr){
    size_t index = *index_ptr;
    buf[index] = '\0';
//only complete first token
    size_t end = 0;
    while (end < index && buf[end] != ' ' && buf[end] != '\t') end++;
//if there are arguments already dont do autocomplete
    if (end != index && end > 0) return;

    char prefix[256];
    if (end >= sizeof(prefix)) return;
    memcpy(prefix, buf, end);
    prefix[end] = '\0';
//exact command is written and exist in that case we show the current directory
    if(command_exact(prefix)){
        printf("\n");
        list_curr_dir();
        redraw_prompt_and_buf(buf, index);
        return;
    }

    char *matches[256];
    int m = find_matches(prefix, matches, 256);
//when no  matches
    if (m ==0){
        printf("\n(no matches)\n");
        redraw_prompt_and_buf(buf, index);
        return;
    }
//when one match is found
    if(m== 1){

        size_t prelen = strlen(prefix);
        size_t fullen = strlen(matches[0]);

        if (fullen > prelen){
            const char *suffix = matches[0] + prelen;
            size_t suf_len = fullen - prelen;
            if (index + suf_len < 511) {
                memcpy(buf + index, suffix, suf_len);
                index += suf_len;
                buf[index] = '\0';
                fwrite(suffix, 1, suf_len, stdout);
                fflush(stdout);

                *index_ptr = index;
            }
        }
        free(matches[0]);
        return;
    }
//multiple matches
    printf("\n");
    for (int i = 0; i < m; i++){
        printf("%s  ", matches[i]);
        free(matches[i]);
    }
    printf("\n");
    redraw_prompt_and_buf(buf, index);
}

// Prompt a command from the user
void prompt( cmd_t *cmd) {
	size_t index = 0;
	char c;
	char buf[512];
	static char oldbuf[512]; // static variable persist through method calls, think like global variable 

    int escape_code_state = 0; // ANSI escape code sequence 1 -> ESC , 2 -> ESC + [

	// tcgetattr gets the parameters of the current terminal
	// STDIN_FILENO will tell tcgetattr that it should write the settings
	// of stdin to oldt
	static struct termios backup_termios, new_termios;
	tcgetattr(STDIN_FILENO, &backup_termios);
	new_termios = backup_termios;

	// ICANON : canonical mode: normally takes care that one line at a time will be processed
	// that means it will return if it sees a "\n" or an EOF or an EOL
	new_termios.c_lflag &= ~(ICANON | ECHO); // disable canonical mode and disable automatic echo

	// Those new settings will be set to STDIN
	// TCSANOW tells tcsetattr to change attributes immediately.

    // if you exit from the process or return from this function
    // without restoring the old settings, the whole terminal will
    // be broken for everybody (including the bash outside)
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

	show_prompt();
	buf[0] = 0;

	while (1) {
		c = getchar();

		// tab
		if (c == 9) {
			do_autocomplete_in_prompt(buf, &index); // autocomplete, we signal autocomplete by adding a question mark to the end
			continue;
		}

		// backspace
		if (c == 127) {
			if (index > 0) {
				putchar(8); // go back 1
	            putchar(' '); // write empty over
            	putchar(8); // go back 1 again
				index--;
			}
			continue;
		}

        // Escap code handler : 27 91 is the escape code: ESC + [
        // see https://en.wikipedia.org/wiki/ANSI_escape_code
        if (c == 27) {
            escape_code_state = 1;
			continue;
		}

        if (escape_code_state == 1 && c == 91) {
            escape_code_state = 2;
			continue;
		}

        // handle ANSI terminal escape codes
        if (escape_code_state == 2) {

            // UP ARROW - currently it has a limited history function, limited to only 1 prior command
		    if (c == 65) {

                         if (history_pos > 0) history_pos--;
                            while (index > 0) { // delete everything
                                    putchar(8); // go back 1
                        putchar(' '); // write empty over
                        putchar(8); // go back 1 again
                                    index--;
                            }

                            if (history_pos < history_count) {
                                   strcpy(buf, history[history_pos]);
                                   printf("%s", buf);
                                    index = strlen(buf);
    }
                    }


            // DOWN ARROW - you might need this for the history feature
                    if (c == 66) {
                 if(history_pos< history_count) history_pos++ ;

        while(index >0){
                putchar(8);
                putchar(' ');
                putchar(8);
                index--;
    }

         if (history_pos <history_count){
                strcpy(buf,history[history_pos]);
                printf("%s",buf);
                index =strlen(buf);
    }
            }
            escape_code_state = 0;
            continue;
        }

		putchar(c); // echo the typed character to screen

		buf[index++] = c; // add the character to buffer
		if (index >= sizeof(buf) - 1) break; // too long

		if (c == '\n') // enter key
			break;

	}

	// trim newline from the end
	if (index > 0 && buf[index - 1] == '\n') index--;	

	buf[index++] = '\0'; // null terminate string

	strcpy(oldbuf, buf);
        normalize_pipes(buf, sizeof(buf));
        cmd->rawline=strdup(buf);
	parse_command(buf, cmd);

	// MUST restore the old settings
	tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
}

void process_command( cmd_t *cmd);

static bool ending_is_sh(const char *s){
        if(!s) return false;
        size_t n =strlen(s);
        return (n>=3 && strcmp(s+ (n-3), ".sh") ==0);
}


static void run_script(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("cant open script");
        exit(1);
    }

    char line[512];

    while (fgets(line, sizeof(line), fp)) {
//removing trailing newlines
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
 }
//empty lines are skipped
        if (len == 0) continue;

        if (line[0] == '#') continue;

        cmd_t *cmd = malloc(sizeof(cmd_t));
        memset(cmd, 0, sizeof(cmd_t));

        parse_command(line, cmd);
        process_command(cmd);
        free_command(cmd);
}

    fclose(fp);
    exit(0);}

static bool is_module_loaded(const char *name){
    FILE *f = fopen("/proc/modules", "r");
    if (!f) return false;

    char line[512];
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
        // /proc/modules line starts with module name
        if(strncmp(line, name, strlen(name)) == 0 && line[strlen(name)]== ' '){
            found =true;
            break;
    }
    }
    fclose(f);
    return found;
}
//for running kernel loading
static int run_sudo_insmod(void){
    pid_t pid =fork();
    if(pid == 0) {
        char *const args[] ={"sudo", "insmod", "./module/mymodule.ko", NULL};
        execv("/usr/bin/sudo", args) ;
        _exit(127);
    }
    int st = 0;
    waitpid(pid, &st,0);
    return st;
}
//for running kernel unloading deleting
static int run_sudo_rmmod(void){
    pid_t pid =fork();
    if(pid ==0) {
          char *const args[]= {"sudo", "rmmod", "mymodule", NULL};
        execv("/usr/bin/sudo", args);
        _exit(127);
    }
    int st = 0;
    waitpid(pid, &st, 0);
    return st;
}
//counting the running stash processes by reading /proc
static int count_running(void) {
    DIR *d = opendir("/proc");
    if (!d) return 1;

    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
//only the numeric dir
        char *endptr = NULL;
        long pid = strtol(ent->d_name, &endptr, 10);
        if (!ent->d_name[0] || *endptr != '\0') continue;

        char path[256];
        snprintf(path, sizeof(path), "/proc/%ld/comm", pid);

        FILE *f = fopen(path, "r");
        if (!f) continue;

        char comm[64];
        if (fgets(comm, sizeof(comm), f)){
            comm[strcspn(comm, "\n")] =0;
            if(strcmp(comm, "stash")==0) count++;
        }
        fclose(f);
    }
    closedir(d);
    return count;
}
//ensuring module loaded on the start
static void check_start(void){
    if (is_module_loaded("mymodule")) {
        printf(" kernel module already loaded.\n");
        return;
    }
    printf("loading kernel module..\n");
    int st = run_sudo_insmod();
    if (st != 0) printf("failed to load module.\n");
}

//if we are the last stash instance then we unload
static void unload_exit(void) {
    int c = count_running();
    if (c <=1){
        if (is_module_loaded("mymodule")){
            printf("unloading kernel module...\n");
            run_sudo_rmmod();
   }
    }
}


int main(int argc, char *argv[]) {
         if(argc ==2 && ending_is_sh(argv[1])){
                run_script(argv[1]);
}

    // TODO: see the top of the source code
    printf("\n%s Shell implemented by %s (%s)",sysname,student1Name,student1Id);
    if(groupSize>1) printf(" and %s (%s)",student2Name,student2Id);
    printf("\n");
	check_start();
	atexit(unload_exit);
	while (1) {
		cmd_t *cmd = malloc(sizeof( cmd_t));		
		memset(cmd, 0, sizeof( cmd_t));  // clear memory

		prompt(cmd);
		process_command(cmd);        

		free_command(cmd);
	}

	printf("\n");
	return 0;
}

//we are resolving cmd name to a  exacutable path
// if cmd contains / we treat it as a path
//if not we search the  path manually
//we return the string with resolved path if found null if not found
static char *resolve_path(const char *name) {
	if (!name || name[0] == '\0') return NULL;
	
	if(strchr(name, '/')) {
		if(is_executable(name)) return strdup(name);
		return NULL;
}		
	const char *path_env=getenv("PATH");
	if(!path_env || path_env[0] =='\0') return NULL;
	char *path_copy =strdup(path_env);
	if(!path_copy) return NULL;

	char *saveptr=NULL;
	for(char *dir= strtok_r(path_copy, ":", &saveptr);
		dir!=NULL;
		dir=strtok_r(NULL,":", &saveptr)){
//building dir/name
		char candidate[512];
		snprintf(candidate, sizeof(candidate), "%s/%s",dir ,name);

	if (is_executable(candidate)) {
		free(path_copy);
		return strdup(candidate);
	}
	}
	free(path_copy);
	return NULL;

}


//for dealing with redirect empty space problems that occur
//when no empty space is there before or after the symbols
static void normalize_redirects(cmd_t *cmd) {
    for (int r = 0; r < 3; r++) {
        if (cmd->redirects[r] && cmd->redirects[r][0] == '\0') {

            int last = cmd->arg_count - 2;
            if (last <= 0 || cmd->args[last] == NULL) return;
            free(cmd->redirects[r]);
            cmd->redirects[r] = strdup(cmd->args[last]);

            free(cmd->args[last]);
            cmd->args[last] = NULL;

            cmd->arg_count -= 1;
        }
    }
}

static void pipes(cmd_t *cmd) {
           int prev_read_fd = -1;
           pid_t pid;

           while(cmd != NULL) {
               int pipefd[2];
//only create pipe if there is a next command
                       if(cmd->next != NULL) {
                    if(pipe(pipefd)< 0){
                        perror("pipe");
                        return;
            }
        }
                  pid =fork();
                   if (pid< 0) {
                 perror("fork");
                 return;
        }
             if (pid == 0) {
//child starts
//if there was a prev pipe read from that
                 if(prev_read_fd != -1) {
                     dup2(prev_read_fd, STDIN_FILENO);
                     close(prev_read_fd);
                 }
//if there is next command write to pipe
               if (cmd->next != NULL){
                        close(pipefd[0]);
                      dup2(pipefd[1], STDOUT_FILENO);
                         close(pipefd[1]);
            }


            char *resolved = resolve_path(cmd->name);
                      if (!resolved) {
                        printf("%s: command not found\n", cmd->name);
                         exit(-1);
            }

            execv(resolved, cmd->args);
            perror("exec");
            exit(1);
        }
//parent part

//parent  no longer need write
                if(cmd->next != NULL) {
                          close(pipefd[1]);
        }
//parent no longer need old read
                 if(prev_read_fd != -1){
                            close(prev_read_fd);
        }
                prev_read_fd =(cmd->next!= NULL) ? pipefd[0] : -1;

                cmd= cmd->next;
    }
//wait for all childs
         while (wait(NULL) > 0);
}



void process_command( cmd_t *cmd) {
        if (cmd->rawline  && cmd->rawline[0] !='\0'){
            if (history_count<200){
                history[history_count++] =strdup(cmd->rawline);
}
            history_pos = history_count;
}

    // built-ins
	if (strcmp(cmd->name, "") == 0) return;
	if (strcmp(cmd->name, "exit") == 0)  exit(0);
	if (strcmp(cmd->name, "cd") == 0) {
		if (cmd->arg_count > 0) 
			if (chdir(cmd->args[1]) == -1)
                printf("- %s: %s  ---  %s\n", cmd->name, strerror(errno),cmd->args[1]);			            		
        return;
	}
        if(strcmp(cmd->name, "history") ==0){
                   for (int i =0;i< history_count; i++) {
               printf("%d  %s\n",i + 1, history[i]);
  }
           return;
}
if(strcmp(cmd->name, "lsfd") ==0){
    if(cmd->arg_count< 2 || !cmd->args[1]){
        printf("usage: lsfd <PID>\n");
        return;
    }
//write Pid into the proc/lsfd
    int wfd = open("/proc/lsfd", O_WRONLY);
    if (wfd <0){
      return;
    }
//write the pid string only
    write(wfd, cmd->args[1], strlen(cmd->args[1]));
    write(wfd, "\n", 1);
    close(wfd);
//read the output

    int rfd =open("/proc/lsfd", O_RDONLY);
    if (rfd <0) {
        return;
    }
    char buf[1024];
    ssize_t n ;
    while ((n = read(rfd, buf, sizeof(buf))) >0){
        write(STDOUT_FILENO, buf,(size_t)n);
    }
    close(rfd);
    return;
}


    // TODO: implement other builtin commands here
    // do not forget to return from this method if cmd was a built-in and already processed
    // otherwise you will continue and fork!

 

    if (cmd->next != NULL){
        // TODO: consider pipe chains
        pipes(cmd);
        return;
    }



    // TODO: implement path resolution here, if you can't locate the
    // command print error message and return before forking!
	char *resolved = resolve_path(cmd->name);
	if(!resolved){
		printf("%s: command not found or not executable\n",cmd->name);
		return;
	}

    // Command is not a builtin then
	pid_t pid = fork();
	if(pid<0){
		printf("fork failed");
		free(resolved);
		return;	


}
normalize_redirects(cmd);
	if (pid == 0) {
        // CHILD
         if (cmd->redirects[0]){
        int fd=open(cmd->redirects[0], O_RDONLY);
                if (fd< 0){
                     perror("input redirection");
                    exit(1);
        }
                 dup2(fd, STDIN_FILENO);
                 close(fd);
    }

         if (cmd->redirects[1]){
                 int fd= open(cmd->redirects[1],O_WRONLY | O_CREAT | O_TRUNC, 0644);
                 if (fd < 0) {
                    perror("output redirection");
                    exit(1);
        }
                 dup2(fd, STDOUT_FILENO);
                 close(fd);
    }

         if (cmd->redirects[2]) {
                int fd = open(cmd->redirects[2], O_WRONLY |O_CREAT |O_APPEND, 0644);
                if (fd < 0) {
                    perror("append redirection");
                    exit(1);
        }
                dup2(fd, STDOUT_FILENO);
                close(fd);
    }

        // This shows how to do exec with auto-path resolve
		// add a NULL argument to the end of args, and the name to the beginning
		// as required by exec

		// TODO: implement exec for the resolved path using execv()
		execv(resolved, cmd->args); // <- DO NOT USE THIS, replace it with execv()
		printf("exec failed");

        // if exec fails print error message
        // normally you should never reach here
        exit(-1);
	} else {
        // PARENT
		free(resolved);
		if(!cmd->background){
			int status=0;
			waitpid(pid,&status,0);
        	} // wait for child process to finish		
	}

}
