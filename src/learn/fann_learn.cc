#include "fann.h"
#include <stdio.h>

int main(int argc, char** argv) {
    if(argc != 5) {
        fprintf(stderr, "usage: %s <data> <model> <num-hidden> <epochs>\n", argv[0]);
        return 1;
    }

    const unsigned int num_input = 324;
    const unsigned int num_output = 1;
    const unsigned int num_layers = 3;
    const unsigned int num_neurons_hidden = atoi(argv[3]);
    const float desired_error = (const float) 0.01;
    const unsigned int max_epochs = atoi(argv[4]);
    const unsigned int epochs_between_reports = 1;

    struct fann *ann = fann_create_standard(num_layers, num_input, num_neurons_hidden, num_output);

    fann_set_activation_function_hidden(ann, FANN_SIGMOID);
    fann_set_activation_function_output(ann, FANN_SIGMOID);

    fann_train_on_file(ann, argv[1], max_epochs, epochs_between_reports, desired_error);

    fann_save(ann, argv[2]);

    fann_destroy(ann);

    return 0;
}
