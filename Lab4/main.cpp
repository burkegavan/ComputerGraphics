
//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"
#include "teapot.h" // teapot mesh

#include <string> 
#include <fstream>
#include <iostream>
#include <sstream>

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID;

unsigned int teapot_vao = 0;
int width = 800;
int height = 600;

GLuint loc1;
GLuint loc2;
GLfloat rotatez = 0.0f;
GLfloat ball_rotate = 0.0f;
GLfloat ball_roll = 0.0f;
GLfloat ball_fall = 20.0f;

int ball_flag = 0;


// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
std::string readShaderSource(const std::string& fileName)
{
	std::ifstream file(fileName.c_str());
	if (file.fail()) {
		std::cout << "error loading shader called " << fileName;
		exit(1);
	}

	std::stringstream stream;
	stream << file.rdbuf();
	file.close();

	return stream.str();
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
	std::string outShader = readShaderSource(pShaderText);
	const char* pShaderSource = outShader.c_str();

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
	// check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
	// Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

	// Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "../Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgramID, "../Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS

void generateObjectBufferTeapot () {
	GLuint vp_vbo = 0;

	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normals");
	
	glGenBuffers (1, &vp_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glBufferData (GL_ARRAY_BUFFER, 3 * teapot_vertex_count * sizeof (float), teapot_vertex_points, GL_STATIC_DRAW);
	GLuint vn_vbo = 0;
	glGenBuffers (1, &vn_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glBufferData (GL_ARRAY_BUFFER, 3 * teapot_vertex_count * sizeof (float), teapot_normals, GL_STATIC_DRAW);
  
	glGenVertexArrays (1, &teapot_vao);
	glBindVertexArray (teapot_vao);

	glEnableVertexAttribArray (loc1);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer (loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray (loc2);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer (loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}


#pragma endregion VBO_FUNCTIONS


void display(){

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.5f, 0.5f, 0.5f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram (shaderProgramID);


	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation (shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation (shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation (shaderProgramID, "proj");
	
	// Hierarchy of Teapots

	// Root of the Hierarchy
	mat4 view = identity_mat4 ();
	mat4 persp_proj = perspective(90, (float)width/(float)height, 0.1, 100.0);
	mat4 local1 = identity_mat4 ();
	local1 = rotate_z_deg (local1, 0.0);
	local1 = translate (local1, vec3 (-50.0, 0.0, -80.0f));


	// for the root, we orient it in global space
	mat4 global1 = local1;
	glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv (matrix_location, 1, GL_FALSE, global1.m);
	glDrawArrays (GL_TRIANGLES, 0, teapot_vertex_count);

	// child of hierarchy

	mat4 local2 = identity_mat4 ();
	local2 = translate (local2, vec3 (0.0, 10.0, 0.0));
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	mat4 global2 = global1*local2;
	glUniformMatrix4fv (matrix_location, 1, GL_FALSE, global2.m);
	glDrawArrays (GL_TRIANGLES, 0, teapot_vertex_count);

	/*
		One to One - global 2 and local 3
	*/

	mat4 local3 = identity_mat4();
	local3 = translate(local3, vec3(0.0, 10.0, 0.0));
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	mat4 global3 = global2*local3;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global3.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);


	/*
		One to Many - Arms and shoulders
		Shoulders are global 3
	*/

	mat4 local4 = identity_mat4();
	local4 = translate(local4, vec3(0.0, 20.0, 0.0));
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	mat4 global4 = global3*local4;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global4.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	//Right arm

	mat4 local5 = identity_mat4();
	local5 = translate(local5, vec3(20.0, 0.0, 0.0));
	mat4 global5 = global3*local5;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global5.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	//Left Arm

	mat4 local6 = identity_mat4();
	local6 = translate(local6, vec3(-20.0, 0.0, 0.0));
	mat4 global6 = global3*local6;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global6.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	/*
		Right leg
	*/
	mat4 local7 = identity_mat4();
	local7 = translate(local7, vec3(5.0, -10.0, 0.0));
	mat4 global7 = global1*local7;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global7.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	mat4 local8 = identity_mat4();
	local8 = translate(local8, vec3(5.0, -10.0, 0.0));
	mat4 global8 = global7*local8;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global8.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	mat4 local9 = identity_mat4();
	local9 = translate(local9, vec3(5.0, -10.0, 0.0));
	mat4 global9 = global8*local9;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global9.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	mat4 local10 = identity_mat4();
	local10 = translate(local10, vec3(5.0, -10.0, 0.0));
	local10 = rotate_z_deg(local10, rotatez);
	mat4 global10 = global9*local10;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global10.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	mat4 local11 = identity_mat4();
	local11 = translate(local11, vec3(10.0, -20.0, 0.0));
	mat4 global11 = global10*local11;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global11.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);
	/*
		Left leg
	*/
	mat4 local12 = identity_mat4();
	local12 = translate(local12, vec3(-5.0, -10.0, 0.0));
	mat4 global12 = global1*local12;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global12.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	mat4 local13 = identity_mat4();
	local13 = translate(local13, vec3(-5.0, -10.0, 0.0));
	mat4 global13 = global12*local13;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global13.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	mat4 local14 = identity_mat4();
	local14 = translate(local14, vec3(-5.0, -10.0, 0.0));
	mat4 global14 = global13*local14;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global14.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	mat4 local15= identity_mat4();
	local15 = translate(local15, vec3(-5.0, -10.0, 0.0));
	mat4 global15 = global14*local15;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global15.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	mat4 local16 = identity_mat4();
	local16 = translate(local16, vec3(-5.0, -10.0, 0.0));
	mat4 global16 = global15*local16;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global16.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	/*
		Ball
	*/

	// Root of the Hierarchy
	mat4 view2 = identity_mat4();
	mat4 persp_proj2 = perspective(90, (float)width / (float)height, 0.1, 100.0);
	mat4 lo1 = identity_mat4();
	lo1 = rotate_z_deg(lo1, ball_rotate);
	lo1 = scale(lo1, vec3(0.75, 0.75, 0.75));
	lo1 = translate(lo1, vec3(ball_roll, ball_fall, -80.0f));


	// for the root, we orient it in global space
	mat4 gl1 = lo1;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj2.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view2.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, gl1.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);
	
	mat4 lo2 = identity_mat4();
	lo2 = translate(lo2, vec3(0.0, -10.0, 0.0));
	mat4 gl2 = gl1*lo2;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, gl2.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	mat4 lo3 = identity_mat4();
	lo3 = translate(lo3, vec3(0.0, 10.0, 0.0));
	mat4 gl3 = gl1*lo3;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, gl3.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	mat4 lo4 = identity_mat4();
	lo4 = translate(lo4, vec3(10.0, 0.0, 0.0));
	mat4 gl4 = gl1*lo4;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, gl4.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	mat4 lo5 = identity_mat4();
	lo5 = translate(lo5, vec3(-10.0, 0.0, 0.0));
	mat4 gl5 = gl1*lo5;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, gl5.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

    glutSwapBuffers();
}



void updateScene() {	

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	//rotatez+=0.2f;
	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// load teapot mesh into a vertex buffer array
	generateObjectBufferTeapot ();
	
}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {

	switch (key) {
	case 'x':
		if (ball_fall > -50.0f && ball_flag == 0) {
			ball_fall -= 2.0f;
			ball_rotate -= 5.0f;
		}
		else {
			ball_flag = 1;
			rotatez += 1.0f;
			ball_roll += 2.0f;
			ball_rotate -= 20.0f;
			ball_fall += 3.0f;
		}
		break;
	
	case 'r':
		rotatez = 0.0f;
		ball_roll = 0.0f;
		ball_rotate = 0.0f;
		ball_fall = 20.0f;
		ball_flag = 0;
		break;
	}

}

int main(int argc, char** argv){

	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("Hello Triangle");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);

	 // A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}











