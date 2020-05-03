#ifndef CONFIG_H_
#define CONFIG_H_

// The number of labels (without negative)
#define label_num 2

struct Config
{

	// This must be the same as seq_length in the src/model_train/config.py
	const int seq_length = 64;

	// The number of expected consecutive inferences for each gesture type.
	const int consecutiveInferenceThresholds[label_num] = {20, 10};

	const char *output_message[label_num] = {
		"knock\r\n",
		"swipe\r\n"};

	const char *debug_state[5] = {"0:info", "1:modesel", "2:songsel", "3:taiko", "4:score"};
};

Config config;
#endif // CONFIG_H_