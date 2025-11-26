#ifndef FEEDBACK
#define FEEDBACK

struct Feedback{
    int feedbackId;
    int custId;
    int reviewStatus;

    char feedback[100];
};

#endif