#include "mbed.h"
#include "uLCD_4DGL.h"

#include "accelerometer_handler.h"
#include "config.h"
#include "magic_wand_model_data.h"

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

//Serial pc(USBTX, USBRX, 115200);
uLCD_4DGL uLCD(D1, D0, D2);
InterruptIn sw2(SW2);
DigitalIn sw3(SW3);
Thread t;
EventQueue queue(32 * EVENTS_EVENT_SIZE);

int state = 0; //0: info  1: modesel  2: songsel  3: taiko  4: score
int flagDraw = 0;
int songCount = 1;
int songSel = 0;
int modeSel = 0;
char* title[8] = { "Happy BDay","-","-","-","-","-","-","-" };


int PredictGesture(float *output) {
	// How many times the most recent gesture has been matched in a row
	static int continuous_count = 0;
	// The result of the last prediction
	static int last_predict = -1;

	// Find whichever output has a probability > 0.8 (they sum to 1)
	int this_predict = -1;
	for (int i = 0; i < label_num; i++)
	{
		if (output[i] > 0.8)
			this_predict = i;
	}

	// No gesture was detected above the threshold
	if (this_predict == -1)
	{
		continuous_count = 0;
		last_predict = label_num;
		return label_num;
	}

	if (last_predict == this_predict)
	{
		continuous_count += 1;
	}
	else
	{
		continuous_count = 0;
	}
	last_predict = this_predict;

	// If we haven't yet had enough consecutive matches for this gesture,
	// report a negative result
	if (continuous_count < config.consecutiveInferenceThresholds[this_predict])
	{
		return label_num;
	}
	// Otherwise, we've seen a positive result, so clear all our variables
	// and report it
	continuous_count = 0;
	last_predict = -1;

	return this_predict;
}

void draw() {
	if(state == 0) {
		uLCD.cls();
		uLCD.locate(1, 1);
		uLCD.color(GREEN);
		uLCD.printf("Music Player");
		uLCD.locate(1, 3);
		uLCD.color(WHITE);
		uLCD.printf(title[songSel]);
	} else if(state == 1) {
		uLCD.cls();
		uLCD.locate(1, 1);
		uLCD.color(GREEN);
		uLCD.printf("Mode Selection");
		uLCD.color(WHITE);
		uLCD.locate(1, 3);
		if(modeSel == 0) uLCD.printf("> ");
		else uLCD.printf("  ");
		uLCD.printf("Forward");
		uLCD.locate(1, 4);
		if (modeSel == 1) uLCD.printf("> ");
		else uLCD.printf("  ");
		uLCD.printf("Backward");
		uLCD.locate(1, 5);
		if (modeSel == 2) uLCD.printf("> ");
		else uLCD.printf("  ");
		uLCD.printf("Change Song");
	} else if(state == 2) {
		uLCD.cls();
		uLCD.locate(1, 1);
		uLCD.color(GREEN);
		uLCD.printf("Song Selection");
		uLCD.color(WHITE);

		for (int i = 0; i < 8; i++) {
			uLCD.locate(1, 3+i);
			if(i == songSel) uLCD.printf("> ");
			else uLCD.printf("  ");
			uLCD.printf(title[i]);
		}
	}
}

void sw2rise() {
	state = 1;
	flagDraw = 1;
}

void serialListener() {

}

int main(int argc, char *argv[]) {
	t.start(callback(&queue, &EventQueue::dispatch_forever));
	queue.call(serialListener);
	sw2.rise(&sw2rise);

	// Initial
	draw();

	constexpr int kTensorArenaSize = 60 * 1024;
	uint8_t tensor_arena[kTensorArenaSize];

	bool should_clear_buffer = false;
	bool got_data = false;

	int gesture_index;

	static tflite::MicroErrorReporter micro_error_reporter;
	tflite::ErrorReporter *error_reporter = &micro_error_reporter;

	const tflite::Model *model = tflite::GetModel(g_magic_wand_model_data);
	if (model->version() != TFLITE_SCHEMA_VERSION)
	{
		//error_reporter->Report(
		//	"Model provided is schema version %d not equal "
		//	"to supported version %d.",
		//	model->version(), TFLITE_SCHEMA_VERSION);
		return;
	}

	static tflite::MicroOpResolver<6> micro_op_resolver;
	micro_op_resolver.AddBuiltin(
		tflite::BuiltinOperator_DEPTHWISE_CONV_2D,
		tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,
								 tflite::ops::micro::Register_MAX_POOL_2D());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,
								 tflite::ops::micro::Register_CONV_2D());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,
								 tflite::ops::micro::Register_FULLY_CONNECTED());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,
								 tflite::ops::micro::Register_SOFTMAX());
	micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
								 tflite::ops::micro::Register_RESHAPE(), 1);

	static tflite::MicroInterpreter static_interpreter(
		model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
	tflite::MicroInterpreter *interpreter = &static_interpreter;

	interpreter->AllocateTensors();

	TfLiteTensor *model_input = interpreter->input(0);
	if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
		(model_input->dims->data[1] != config.seq_length) ||
		(model_input->dims->data[2] != kChannelNumber) ||
		(model_input->type != kTfLiteFloat32))
	{
		//error_reporter->Report("Bad input tensor parameters in model");
		return;
	}

	int input_length = model_input->bytes / sizeof(float);

	TfLiteStatus setup_status = SetupAccelerometer(error_reporter);
	if (setup_status != kTfLiteOk)
	{
		error_reporter->Report("Set up failed\n");
		return;
	}

	error_reporter->Report("Set up successful...\n");

	while (true) {
		got_data = ReadAccelerometer(error_reporter, model_input->data.f, input_length, should_clear_buffer);

		if (!got_data)
		{
			should_clear_buffer = false;
			continue;
		}

		TfLiteStatus invoke_status = interpreter->Invoke();
		if (invoke_status != kTfLiteOk)
		{
			error_reporter->Report("Invoke failed on index: %d\n", begin_index);
			continue;
		}

		gesture_index = PredictGesture(interpreter->output(0)->data.f);

		should_clear_buffer = gesture_index < label_num;

		if (gesture_index < label_num)
		{
			//error_reporter->Report(config.output_message[gesture_index]); // TO REMOVE
			error_reporter->Report(config.debug_state[state]);

			if(state == 1) {
				if (gesture_index == 0) {
					if(modeSel > 0) {
						modeSel--;
						flagDraw = 1;
					}
				} else if (gesture_index == 1) {
					if(modeSel < 2) {
						modeSel++;
						flagDraw = 1;
					}
				}
			}
		}

		if(!sw3) {
			if(state == 1) {
				if(modeSel == 0) {
					// fwd
					state = 0;
					flagDraw = 1;
				} else if(modeSel == 1) {
					// bwd
					state = 0;
					flagDraw = 1;
				} else if(modeSel == 2) {
					// chg song
					state = 2;
					flagDraw = 1;
				}
			}
		}

		if(flagDraw == 1) {
			draw();
			flagDraw = 0;
		}
	}
}