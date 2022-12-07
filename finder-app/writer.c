#include <stdio.h>
#include <syslog.h>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("error: must provide filename and string to be written\n");
        printf("usage: writer <path> <string>\n");
        return 1;
    }

    const char* filename = argv[1];
    const char* output = argv[2];

    FILE* file = fopen(filename, "w");

    if (file)
    {
        syslog(LOG_DEBUG, "Writing %s to %s", output, filename);
        fprintf(file, "%s", output);
        fclose(file);
    }
    else
    {
        syslog(LOG_ERR,"error: failed to open file %s", filename);
        return 1;
    }

    return 0;
}
