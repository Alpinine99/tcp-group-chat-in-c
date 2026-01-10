
// Formatting constants for terminal output
#define RED "\x1b[31m"
#define RESET "\x1b[0m"
#define GREEN "\x1b[32m"
#define BOLD "\x1b[1m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define WHITE "\x1b[37m"
#define UNDERLINE "\x1b[4m"

// Networking constants
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define MAX_USERNAME_LEN 15
#define MIN_USERNAME_LEN 5
#define MAX_MESSAGE_LEN 256

// Control sequences for special commands
#define INCLUDE "#!<INCLUDE>!#"
#define EXCLUDE "#!<EXCLUDE>!#"
#define QUIT "#!<QUIT>!#"
#define SERVER_ALIAS "#!<SERVER>!#"