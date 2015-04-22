////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//

/*This project is for simulating ocean waves in 3D using the summation of multiple sine waves*/

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace std;

namespace octet {

	
	class Ocean_Waves : public app {

		//scene for waves
		ref<visual_scene> app_scene;
		ref<text_overlay> overlay;
		ref<mesh_text> text;


	public:
		/// this is called when we construct the class before everything is initialised.
		Ocean_Waves(int argc, char **argv) : app(argc, argv) {
		}

	private:

		//meshbuilder helps to construct a polygon by specifying vertices
		mesh_builder ocean_plane;

		//the mesh that will be created by the meshbuilder
		mesh* water;
	
		//array for keeping track of size of ocean polygon - numbers are x and z coordinates 
		//i.e. vertices[21][34] = y, will be in coordinate space (21, y, 34) where y is the height 
		//to be calculated by sine wave formula
		float vertices[64][64];

		//vertices for simulating the ocean each frame
		vec4 new_vertices[4];
		vec4 normals[4];
		vec2 uvs[4];

		material* solid;
		material* textured;

		//number of different sine waves
		int number_of_waves = 8; 

		//time
		float delta_time = 0.015f;

		//keeps track of current frame for use in sine wave formula
		int frame = 0;
		const int max_frame = 100000; //reset if hit maximum frame

		//the file containing the configurable information
		ifstream config_file;
		ofstream overwrite_file;

		//to take in each value from the file
		std::string file_amplitude, file_direction_x, file_direction_y, file_frequency, file_phase;

		//array of parameters for the sine waves -  all are configurable
		float amplitude[8];  
		vec2 direction[8]; 
		float frequency[8]; 
		float phase[8]; 

		int sine_wave = 1;

		//is it wire frame?
		bool wireframe_mode = false;
		bool non_textured_mode = false;

		void app_init() {

			//initialise scene
			app_scene = new visual_scene();
			app_scene->create_default_camera_and_lights();
			app_scene->get_camera_instance(0)->set_far_plane(10000);
			
			//create node
			scene_node* ocean_node = app_scene->add_scene_node();

			material *black = new material(vec4(0, 0, 0, 1));
			material* blue_tex = new material(new image("assets/Water.gif"));
		
			mesh_box* skybox = new mesh_box(vec3(100, 100, 100));
			
			//initialise plane and create it 32x32 in size
			initialise();
			ocean_plane.init();
			create_plane();
			water = new mesh();
			ocean_plane.get_mesh(*water);
			water->set_mode(GL_TRIANGLES);
			
			//add the initial mesh instance
			app_scene->add_mesh_instance(new mesh_instance(ocean_node, water, blue_tex));
			app_scene->add_mesh_instance(new mesh_instance(ocean_node, skybox, black));
	

			//set up camera to be able to view entire plane
			scene_node *camera_node = app_scene->get_camera_instance(0)->get_node();
			scene_node *light_node = app_scene->get_light_instance(0)->get_node();
			camera_node->translate(vec3(30, 0, 0));
			camera_node->translate(vec3(0, 15, 0));
			camera_node->translate(vec3(0, 0, 50));
			camera_node->rotate(-15, vec3(1, 0, 0));
			light_node->rotate(-70, vec3(0, 1, 0));
		}

		//load in values from config files
		void load_files()
		{
			for (int i = 0; i < number_of_waves; i++)
			{
				std::string fileNumber = std::to_string(i+1);
				config_file.open("config/" + fileNumber + ".txt");
				getline(config_file, file_amplitude);
				getline(config_file, file_direction_x);
				getline(config_file, file_direction_y);
				getline(config_file, file_frequency);
				getline(config_file, file_phase);
				config_file.close();

				amplitude[i] = atof(file_amplitude.c_str());
				direction[i].x() = atof(file_direction_x.c_str());
				direction[i].y() = atof(file_direction_y.c_str());
				frequency[i] = atof(file_frequency.c_str());
				phase[i] = atof(file_phase.c_str());
			}
		}

		//overwrites the text files with new values shown in demo
		void overwrite_files()
		{
			for (int i = 0; i < number_of_waves; i++)
			{
				std::string fileNumber = std::to_string(i+1);
				overwrite_file.open("config/" + fileNumber + ".txt");
				overwrite_file << std::to_string(amplitude[i])<<"\n";
				overwrite_file << std::to_string(direction[i].x())<<"\n";
				overwrite_file << std::to_string(direction[i].y()) << "\n";
				overwrite_file << std::to_string(frequency[i]) << "\n";
				overwrite_file << std::to_string(phase[i]) << "\n";
				overwrite_file.close();
			}
		}


		void initialise()
		{
			load_files();
			create_text();

			//initialise the vertices array to 1.0f so to start with every vertex has height of 1.0f
			for (int i = 0; i != 64; i++)
			{
				for (int j = 0; j != 64; j++)
				{
					vertices[i][j] = 1.0f;
				}

			}
		}

		//creates the initial plane 
		void create_plane(){

			//the start up normal is the same for all vertices
			vec4 normal = vec4(0, 0, 1, 0);
			
			//use the number of vertices to keep track of indices
			int number_of_vertices;

			for (int i = 0; i < 60; i++)
			{
				for (int j = 0; j < 60; j++)
				{
					number_of_vertices = ocean_plane.get_vertices();

					ocean_plane.add_vertex(vec4(j, vertices[j][i], i, 1), normal, 0, 0);
					ocean_plane.add_vertex(vec4(j, vertices[j][i + 1], i + 1.0f, 1), normal, 1, 0);
					ocean_plane.add_vertex(vec4(j + 1.0f, vertices[j + 1][i + 1], i + 1.0f, 1), normal, 1, 1);
					ocean_plane.add_vertex(vec4(j + 1.0f, vertices[j + 1][i], i, 1), normal, 0, 1);
				

					ocean_plane.add_index(number_of_vertices);
					ocean_plane.add_index(number_of_vertices + 1);
					ocean_plane.add_index(number_of_vertices + 2);
					ocean_plane.add_index(number_of_vertices);
					ocean_plane.add_index(number_of_vertices + 2);
					ocean_plane.add_index(number_of_vertices + 3);
				}
			}
		}
		
		float get_wave_height(int x_position, int z_position)
		{
			float wave_height = 0.0f;

			for (int i = 0; i < number_of_waves; i++)
			{
				//sum all wave heights taking into account x and z coordinates of each vertex 
				wave_height += wave(i, x_position, z_position);		
			}

			return wave_height;

		}
		
		float wave(int wave_number, int x, int z)
		{
			//calculate angle of new wave based on horizontal crest of vertex using x and z coordinates of vertices

			//first calculate x and z directions
			float x_offset = x - direction[wave_number].x();
			float z_offset = z - direction[wave_number].y();

			//then find angle between them
			float theta = sqrt(x_offset*x_offset + z_offset*z_offset);

			//use sine wave formula to return a height of a wave
			return amplitude[wave_number] * sin((frequency[wave_number] * theta - frame * phase[wave_number]) * delta_time);
		}


		void make_waves()
		{
			//set all the vertices in accordance with the sine wave calculation
			for (int i = 0; i != 64; i++)
			{
				for (int j = 0; j != 64; j++)
				{
					//set a new y value (height) for the vertex at location (x, z) - in this case i and j
					vertices[i][j] = get_wave_height(i, j);
				}
				
			}
		}

		void simulate_ocean()
		{
			//reset the vertices dyn array so back to nothing
			ocean_plane.reset_vertices();
			
			for (int i = 0; i < 60; i++)
			{
				for (int j = 0; j < 60; j++) 
				{
					//create new vertices based on ones made using sine wave
					new_vertices[0] = vec4(j, vertices[j][i], i, 1);
					new_vertices[1] = vec4(j, vertices[j][i + 1], i + 1.0f, 1);
					new_vertices[2] = vec4(j + 1.0f, vertices[j + 1][i + 1], i + 1.0f, 1);
					new_vertices[3] = vec4(j + 1.0f, vertices[j + 1][i], i, 1);
					
					//calculate normal for each triangle surface using cross product of two of the triangle edges
				 	normals[0] = (new_vertices[3] - new_vertices[0]).cross(new_vertices[1] - new_vertices[0]);
					normals[1] = (new_vertices[2] - new_vertices[3]).cross(new_vertices[0] - new_vertices[3]);
					normals[2] = (new_vertices[1] - new_vertices[2]).cross(new_vertices[3] - new_vertices[2]);
					normals[3] = (new_vertices[0] - new_vertices[1]).cross(new_vertices[2] - new_vertices[1]);
					

					//calculate uvs using time step
					uvs[0] = vec2(j * delta_time, i * delta_time);
					uvs[1] = vec2(j * delta_time, i * delta_time + delta_time);	
					uvs[2] = vec2(j * delta_time + delta_time, i * delta_time + delta_time);
					uvs[3] = vec2(j * delta_time + delta_time, i * delta_time);

					//add vertex in with new normals and uvs
					ocean_plane.add_vertex(new_vertices[0], normals[0], uvs[0].x(), uvs[0].y());
					ocean_plane.add_vertex(new_vertices[1], normals[1], uvs[1].x(), uvs[1].y());
					ocean_plane.add_vertex(new_vertices[2], normals[2], uvs[2].x(), uvs[2].y());
					ocean_plane.add_vertex(new_vertices[3], normals[3], uvs[3].x(), uvs[3].y());

				}
			}
		}

		//creates a new mesh text in a certain size text box
		void create_text(){
			overlay = new text_overlay();
			bitmap_font *font = overlay->get_default_font(); //get font
			aabb text_box(vec3(-300.0f, 150.0f, 0.0f), vec3(300, 200, 0)); //position , half extent
			text = new mesh_text(font, "", &text_box); //create text of certain font in box
			overlay->add_mesh_text(text);
		}

		//shows what text to be shown on screen
		void draw_text(){

				text->clear();
				text->format(
					"Sine wave: %d\n"
					"Amplitude: %g\n"
					"Direction: (%g, %g)\n"
					"Frequency: %g\n"
					"Phase: %g\n"
					"Use 'NUMBER KEYS' to select sine wave\n"
					"Use 'Q' and 'A'keys to adjust amplitude\n"
					"Use 'W' and 'S' keys to adjust frequency\n"
					"Use 'E' and 'D' keys to adjust phase\n"
					"Use 'DIRECTION' keys to adjust direction\n"
					"Press 'O' to turn on wireframe mode and press 'P' to turn it off\n"
					"Press 'K' to see solid non-textured and press 'L' to see textured\n"
					"Press 'SPACE' to save values to files\n",
					sine_wave,
					amplitude[sine_wave - 1],
					direction[sine_wave - 1].x(),
					direction[sine_wave - 1].y(),
					frequency[sine_wave - 1],
					phase[sine_wave - 1]);
		}

		//hotkey controller
		void controller()
		{
			//amplitude
			if (is_key_down('Q'))
			{
				amplitude[sine_wave - 1] += 0.5f;
			}

			else if (is_key_down('A'))
			{
				amplitude[sine_wave - 1] -= 0.5f;
			}
			
			//frequency
			else if (is_key_down('W'))
			{
				frequency[sine_wave - 1] += 0.5f;
			}

			else if (is_key_down('S'))
			{
				frequency[sine_wave - 1] -= 0.5f;
			}

			//phase
			else if (is_key_down('E'))
			{
				phase[sine_wave - 1] += 0.5f;
			}

			else if (is_key_down('D'))
			{
				phase[sine_wave - 1] -= 0.5f;
			}
			
			//wireframe mode
			else if (is_key_down('O'))
			{
				wireframe_mode = true;
			}

			else if (is_key_down('P'))
			{
				wireframe_mode = false;
			}

			//textured mode
			else if (is_key_down('K'))
			{
				material* solid = new material(vec4(0.0f, 0.5f, 1.0f, 1));
				app_scene->get_mesh_instance(0)->set_material(solid);
			}

			else if (is_key_down('L'))
			{
				material* textured = new material(new image("assets/Water.gif"));
				app_scene->get_mesh_instance(0)->set_material(textured);
			}

			//direction
			else if (is_key_down(key_up))
			{
				direction[sine_wave - 1].y() += 0.5f;
			}

			else if (is_key_down(key_down))
			{
				direction[sine_wave - 1].y() -= 0.5f;
			}

			else if (is_key_down(key_left))
			{
				direction[sine_wave - 1].x() -= 0.5f;
			}

			else if (is_key_down(key_right))
			{
				direction[sine_wave - 1].x() += 0.5f;
			}

			//overwrite config files
			else if (is_key_down(key_space))
			{
				overwrite_files();
			}

			//chooseing sine wave
			else if (is_key_down('1'))
			{
				sine_wave = 1;
			}

			else if (is_key_down('2'))
			{
				sine_wave = 2;
			}

			else if (is_key_down('3'))
			{
				sine_wave = 3;
			}

			else if (is_key_down('4'))
			{
				sine_wave = 4;
			}

			else if (is_key_down('5'))
			{
				sine_wave = 5;
			}

			else if (is_key_down('6'))
			{
				sine_wave = 6;
			}

			else if (is_key_down('7'))
			{
				sine_wave = 7;
			}

			else if (is_key_down('8'))
			{
				sine_wave = 8;
			}
		
		}

		void draw_world(int x, int y, int w, int h) {

			int vx = 0, vy = 0;
			get_viewport_size(vx, vy);

			//reset frame to 0 if it goes over 100000
			if (frame < max_frame)
			{
				//to keep track of frames and used in sine wave function
				frame++;
			}
			else frame = 0;
		
			//for hotkeys
			controller();

			//make waves sets all y values in vertices array 
			make_waves();

			//use vertices array to build mesh
			simulate_ocean();

			app_scene->begin_render(vx, vy);

			// update matrices. assume 30 fps.
			app_scene->update(1.0f / 30);

			//convert to a mesh
			ocean_plane.get_mesh(*water);

			// draw the scene
			app_scene->render((float)vx / vy);

			//if wireframe mode is on show wireframe
			if (wireframe_mode)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}
			else
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				//draw the text on the screen
				draw_text();

				// convert it to a mesh.
				text->update();

				// draw the text overlay
				overlay->render(vx, vy);
			}	
		}
	};
}
