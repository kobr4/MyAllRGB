#include <FL/Fl.H>
#include <FL/Fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Timer.H>
#include <apr.h>
#include <apr_thread_proc.h>
#include "AllRGB.h"


Fl_RGB_Image * g_image = NULL;
Fl_Box *g_box = NULL; 
Fl_Window * g_window = NULL;
apr_thread_t * g_main_thread;
apr_status_t g_main_status;

typedef struct {
	Fl_Int_Input * redInput;
	Fl_Int_Input * greenInput;
	Fl_Int_Input * blueInput;
	Fl_Int_Input * widthInput;
	Fl_Int_Input * heightInput;
	Fl_Int_Input * nbSeedInput;
	Fl_Int_Input * nbThreadInput;
	Fl_Timer * timer;
	Fl_Button * generateButton;
} T_MAINPANEL;
T_MAINPANEL g_mainpanel;

void refresh_image() {
	if (g_image != NULL) {
		g_image->uncache();
		g_box->redraw();
	}
}

void * func_message_fin(void *data) {
	fl_alert("ouput.tga has been generated");
	return 0;
};


static void * APR_THREAD_FUNC thread_function( apr_thread_t * thread, void *data )
{
  allrgb_generate();
  Fl::awake ((Fl_Awake_Handler)func_message_fin);
  return NULL;
}

void set_job_from_panel() {
	g_allrgbjob.r_range = atoi(g_mainpanel.redInput->value());
	g_allrgbjob.g_range = atoi(g_mainpanel.greenInput->value());
	g_allrgbjob.b_range = atoi(g_mainpanel.blueInput->value());
	g_allrgbjob.width = atoi(g_mainpanel.widthInput->value());
	g_allrgbjob.height = atoi(g_mainpanel.heightInput->value());
	g_allrgbjob.nb_thread = atoi(g_mainpanel.nbThreadInput->value());
	g_allrgbjob.nb_seeds = atoi(g_mainpanel.nbSeedInput->value());
}

void timer_stop() {
	g_mainpanel.timer->suspended(1);
}

void start_generate(Fl_Widget*, void*) {
	if (g_allrgbjob.status == 1) {
		Fl::error("Can't launch this process more than once asshole !");
		return;
	}

	g_mainpanel.timer->value(50*60*60.0);
	g_mainpanel.timer->suspended(0);

	set_job_from_panel();

	if( apr_thread_create(&g_main_thread, NULL, thread_function, NULL,  g_pool) != APR_SUCCESS ) 
	{
		printf( "Could not create the thread\n");
		exit( -1);
	}

	g_allrgbjob.status = 1;

	g_window->remove(g_box);
	g_box->deimage();
	if (g_buffer_image != NULL)
		free(g_buffer_image);

	if (g_box != NULL) 
		delete g_box;

	if (g_image != NULL)
		delete g_image;

	g_buffer_image = (unsigned char*)calloc(g_allrgbjob.width*g_allrgbjob.height*3, sizeof(unsigned char));
	g_box = new Fl_Box(170,0,g_allrgbjob.width,g_allrgbjob.height,"");
	g_image = new Fl_RGB_Image(g_buffer_image, g_allrgbjob.width, g_allrgbjob.height);
	g_box->image(g_image);
	g_window->add(g_box);

	int w = g_allrgbjob.width > 640 ? g_allrgbjob.width : 640;
	int h = g_allrgbjob.height > 480 ? g_allrgbjob.height : 480;
	g_window->size(w+160,h+0);
}


void init_main_panel() {
	g_mainpanel.redInput = new Fl_Int_Input(85,10,80,25,"Red range");
	g_mainpanel.redInput->value("100");
	g_window->add(g_mainpanel.redInput);

	g_mainpanel.greenInput = new Fl_Int_Input(85,40,80,25,"Green range");
	g_mainpanel.greenInput->value("100");
	g_window->add(g_mainpanel.greenInput);

	g_mainpanel.blueInput = new Fl_Int_Input(85,70,80,25,"Blue range");
	g_mainpanel.blueInput->value("100");
	g_window->add(g_mainpanel.blueInput);

	g_mainpanel.widthInput = new Fl_Int_Input(85,100,80,25,"Width");
	g_mainpanel.widthInput->value("1280");
	g_window->add(g_mainpanel.widthInput);

	g_mainpanel.heightInput = new Fl_Int_Input(85,130,80,25,"Height");
	g_mainpanel.heightInput->value("720");
	g_window->add(g_mainpanel.heightInput);

	g_mainpanel.nbSeedInput = new Fl_Int_Input(85,160,80,25,"Nb seeds");
	g_mainpanel.nbSeedInput->value("1");
	g_window->add(g_mainpanel.nbSeedInput);

	g_mainpanel.nbThreadInput = new Fl_Int_Input(85,190,80,25,"Nb threads");
	g_mainpanel.nbThreadInput->value("2");
	g_window->add(g_mainpanel.nbThreadInput);

	g_mainpanel.generateButton = new Fl_Button(10,220,100,25,"Generate !");
	g_mainpanel.generateButton->callback((Fl_Callback_p)&start_generate);
	g_window->add(g_mainpanel.generateButton);

	g_mainpanel.timer = new Fl_Timer(FL_VALUE_TIMER, 85, 250, 80, 25, "Time");


	g_mainpanel.timer->suspended(!0);
	g_mainpanel.timer->direction(1);
	g_window->add(g_mainpanel.timer);

}



int main(int argc, char** argv)
{
	if (apr_initialize() != APR_SUCCESS) 
	{
		printf( "Could not initialize\n");
		exit(-1);
	}

	if (apr_pool_create(&g_pool, NULL) != APR_SUCCESS) 
	{
		printf( "Could not allocate pool\n");
		exit( -1);
	}
	Fl::lock();
	g_window = new Fl_Window(640,480,"MyAllRGB");
	init_main_panel();
	g_window->end();
	g_window->show(argc, argv);

	return Fl::run();
}
