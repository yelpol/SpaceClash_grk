#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <vector>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"
#include "Texture.h"

GLuint programColor;
GLuint programTexture;
GLuint programSkybox;
// GLuint programSun; //próba stworznia osobnych shaderów do słońca, jak w cw.4 (w trakcie)

Core::Shader_Loader shaderLoader;

obj::Model shipModel;
obj::Model sphereModel;

glm::vec3 cameraPos = glm::vec3(-5, 0, 0);
glm::vec3 cameraDir; // Wektor "do przodu" kamery
glm::vec3 cameraSide; // Wektor "w bok" kamery
float cameraAngle = 0;

// glm::vec3 lightPos = glm::vec3(0, 0, 0);

glm::mat4 cameraMatrix, perspectiveMatrix;

GLuint textureAsteroid;
GLuint textureSkybox;
GLuint textureSun;

glm::vec3 sunPosition = glm::vec3(0);

static const int NUM_ASTEROIDS = 10;
glm::vec3 asteroidPositions[NUM_ASTEROIDS];

float xprev = 0.1;
float yprev = 0.1;

float xdiff = 0.0f;
float ydiff = 0.0f;
float zdiff = 0.0f;

glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

void keyboard(unsigned char key, int x, int y)
{
	float angleSpeed = 0.1f;
	float moveSpeed = 0.1f;
	switch(key)
	{
	case 'z': zdiff = -angleSpeed; break;
	case 'x': zdiff = angleSpeed; break;
	case 'w': cameraPos += cameraDir * moveSpeed; break;
	case 's': cameraPos -= cameraDir * moveSpeed; break;
	case 'd': cameraPos += cameraSide * moveSpeed; break;
	case 'a': cameraPos -= cameraSide * moveSpeed; break;
	}
}

void mouse(int x, int y)
{
	xdiff = x - xprev;
	ydiff = y - yprev;

	xprev = x;
	yprev = y;
}

glm::mat4 createCameraMatrix()
{
	glm::quat rotationX = glm::angleAxis(xdiff*0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat rotationY = glm::angleAxis(ydiff*0.01f, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat rotationZ = glm::angleAxis(zdiff, glm::vec3(0.0f, 0.0f, 1.0f));


	glm::quat rotationChange = rotationX * rotationY * rotationZ;
	rotation = glm::normalize(rotationChange * rotation);

	xdiff = 0;
	ydiff = 0;
	zdiff = 0;

	cameraDir = glm::inverse(rotation) * glm::vec3(0, 0, -1);
	cameraSide = glm::inverse(rotation) * glm::vec3(1, 0, 0);
	return Core::createViewMatrixQuat(cameraPos, rotation);
}

void drawObjectColor(obj::Model * model, glm::mat4 modelMatrix, glm::vec3 color, glm::vec3 lightDir)
{
	GLuint program = programColor;

	glUseProgram(program);

	glUniform3f(glGetUniformLocation(program, "objectColor"), color.x, color.y, color.z);
	//glUniform3f(glGetUniformLocation(program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	Core::DrawModel(model);

	glUseProgram(0);
}

void drawObjectTexture(obj::Model * model, glm::mat4 modelMatrix, GLuint textureId,  glm::vec3 lightDir)
{
	GLuint program = programTexture;

	glUseProgram(program);

	glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
	Core::SetActiveTexture(textureId, "textureSampler", program, 0);
	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	//glUniform3f(glGetUniformLocation(program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	Core::DrawModel(model);

	glUseProgram(0);
}

void drawSkybox(obj::Model * model, glm::mat4 modelMatrix, GLuint textureId)
{
	GLuint program = programSkybox;
	glUseProgram(program);

	Core::SetActiveTexture(textureId, "textureSampler", program, 0);
	//glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	Core::DrawModel(model);

	glUseProgram(0);
}

void drawSun(glm::vec3 pos)
{
	sunPosition = pos;
	glm::vec3 lightDir = glm::normalize(sunPosition - cameraPos);
	drawObjectTexture(&sphereModel, glm::translate(sunPosition), textureSun, lightDir);
}
// void drawSun(obj::Model * model, glm::mat4 modelMatrix, GLuint textureId)
// {
//
// 	GLuint program = programSun;
// 	glUseProgram(program);
//
// 	Core::SetActiveTexture(textureId, "textureSampler", program, 0);
// 	glUniform3f(glGetUniformLocation(programSun, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
//
// 	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
// 	glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
// 	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
//
// 	Core::DrawModel(model);
//
// 	glUseProgram(0);
// }


void renderScene()
{
	// Aktualizacja macierzy widoku i rzutowania
	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.1f, 0.3f, 1.0f);

	drawSkybox(&sphereModel, glm::translate(cameraPos)*glm::scale(glm::vec3(30)), textureSkybox);

	glm::mat4 shipInitialTransformation = glm::translate(glm::vec3(0,-0.25f,0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0,1,0)) * glm::scale(glm::vec3(0.25f));
	//glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::rotate(-cameraAngle, glm::vec3(0,1,0)) * shipInitialTransformation;
	glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * shipInitialTransformation;

	// glm::mat4 sunMatrix = glm::translate(glm::vec3(0, 0, 0));
	// drawSun(&sphereModel, sunMatrix, textureSun);
	glm::vec3 lightDir = glm::normalize(cameraPos - sunPosition);
	drawObjectColor(&shipModel, shipModelMatrix, glm::vec3(0.6f), lightDir);

	for (int i = 0; i < NUM_ASTEROIDS; i++)
	{
		drawObjectTexture(&sphereModel, glm::translate(asteroidPositions[i]), textureAsteroid, lightDir);
	}

	//glm::mat4 sunMatrix = glm::translate(lightPos);
	//drawSun(&sphereModel, sunMatrix, textureSun);
	drawSun(sunPosition);

	glutSwapBuffers();
}

void init()
{
	srand(time(0));
	glEnable(GL_DEPTH_TEST);
	programColor = shaderLoader.CreateProgram("shaders/shader_color.vert", "shaders/shader_color.frag");
	programTexture = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
	programSkybox = shaderLoader.CreateProgram("shaders/shader_sky.vert", "shaders/shader_sky.frag");
	//programSun = shaderLoader.CreateProgram("shaders/shader_4_sun.vert", "shaders/shader_4_sun.frag");

	sphereModel = obj::loadModelFromFile("models/sphere.obj");
	shipModel = obj::loadModelFromFile("models/spaceship.obj");
	textureAsteroid = Core::LoadTexture("textures/asteroid.png");
	for(int i = 0; i < NUM_ASTEROIDS; i++)
	{
		asteroidPositions[i] = glm::ballRand(10.f);
	}
	textureSkybox = Core::LoadTexture("textures/sky.png");
	textureSun = Core::LoadTexture("textures/sun.png");
}

void shutdown()
{
	shaderLoader.DeleteProgram(programColor);
	shaderLoader.DeleteProgram(programTexture);
	shaderLoader.DeleteProgram(programSkybox);
}

void idle()
{
	glutPostRedisplay();
}

int main(int argc, char ** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(200, 200);
	glutInitWindowSize(600, 600);
	glutCreateWindow("OpenGL Pierwszy Program");
	glewInit();

	init();
	glutKeyboardFunc(keyboard);
	glutPassiveMotionFunc(mouse);
	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutMainLoop();

	shutdown();

	return 0;
}
