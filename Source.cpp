#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#define	GLEW_STATIC
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "meshes.h"
#include "camera.h"

//image loader for textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "Module 5 Scene"; // Window Title

	// Variables for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	//camera initial settings
	// camera
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	bool firstMouse = true;

	float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
	float pitch = 0.0f;
	float lastX = WINDOW_WIDTH / 2.0;
	float lastY = WINDOW_HEIGHT / 2.0;
	float fov = 45.0f;
	float sensitivity = 0.03f; // change this to a global with a default; to be controlled by scroll

	// timing
	float deltaTime = 0.0f;	// time between current frame and last frame
	float lastFrame = 0.0f;

	// Stores the GL data relative to a given mesh
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbos[2];     // Handles for the vertex buffer objects
		GLuint nIndices;    // Number of indices of the mesh
		GLuint nVertices;
	};

	// Main GLFW window
	GLFWwindow* gWindow = nullptr;
	// Shader program
	GLuint gProgramId;
	GLuint lampProgramId;
	//Shape Meshes from Professor Brian
	Meshes meshes;
	// Camera Variables
	Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	bool isPerspectiveView = true;
	bool pKeyPressed = false;
	// Texture id
	GLuint gTexture0, gTexture1, gTexture2, gTexture3;

	//GLMesh gMesh;

	// Texture id
	GLuint gTextureId;
	glm::vec2 gUVScale(1.0f, 1.0f);

	// Shader programs
	GLuint gCubeProgramId;
	GLuint gLampProgramId;
	;

	// Cube and light color
	glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
	glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

	// Light1 position and scale
	glm::vec3 gLightPosition;
	glm::vec3 gLightScale(0.3f);

	// positions of the point lights
	glm::vec3 pointLightPositions[] = {
		glm::vec3(0.0f, -1.0f, -2.0f),
		glm::vec3(1.5f, 3.0f, -5.0f),
		glm::vec3(-2.5f, -4.0f, 2.0f),
		glm::vec3(-3.5f, 2.0f, 9.0f),
		glm::vec3(-1.5f, 3.0f, 4.0f),
	};
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char*[], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint &programId);
void UDestroyShaderProgram(GLuint programId);

//textures
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);

// Mouse Input
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec2 vertexTextureCoordinate;
out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader

//Global variables for the transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
	//gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
	//vertexTextureCoordinate = textureCoordinate;
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

	vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

	vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
	vertexTextureCoordinate = textureCoordinate;
}
);



/* Cube Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
	out vec4 fragmentColor; // For outgoing cube color to the GPU

in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 keyColor;
uniform vec3 keyPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;
uniform float ambientStrength = 0.1f; // Set ambient or global lighting strength
uniform float specularIntensity = 0.5f; // Set specular light strength
uniform float highlightSize = 32.0f; // Set specular highlight size

void main()
{
	/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/
	//Calculate Ambient lighting*/
	vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

	////Calculate Diffuse lighting*/
	vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
	vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	vec3 keyDirection = normalize(keyPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube

	float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	float keyImpact = max(dot(norm, keyDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light

	vec3 diffuse = impact * lightColor; // Generate diffuse light color
	vec3 keyDiffuse = keyImpact * lightColor; // Generate diffuse light color

	//Calculate Specular lighting*/

	vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
	vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector

	//Calculate specular component
	float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
	vec3 specular = specularIntensity * specularComponent * lightColor;
	vec3 keySpecular = specularIntensity * specularComponent * lightColor;

	//Texture holds the color to be used for all three components
	vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

	//Calculate phong result
	vec3 phong = (ambient + keyDiffuse + diffuse + specular + keySpecular) * textureColor.xyz;

	fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

	out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
	fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Create the mesh
	//UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object
	meshes.CreateMeshes();

	// Create the shader program
	if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
		return EXIT_FAILURE;


	// Load texture 0
	const char* texFilename = "black.jpg";
	if (!UCreateTexture(texFilename, gTexture0))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	// Load texture 1
	texFilename = "orange-ruler.jpg";
	if (!UCreateTexture(texFilename, gTexture1))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	// Load texture 2
	texFilename = "brick_wall.jpg";
	if (!UCreateTexture(texFilename, gTexture2))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	// Load texture 3
	texFilename = "wood_grain.jpg";
	if (!UCreateTexture(texFilename, gTexture3))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}


	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	glUseProgram(gProgramId);

	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(gWindow))
	{
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;
		// input
		// -----
		UProcessInput(gWindow);

		// Render this frame
		URender();

		glfwPollEvents();
	}

	// Release mesh data
	meshes.DestroyMeshes();

	// Release texture
	UDestroyTexture(gTexture1);
	UDestroyTexture(gTexture2);

	// Release shader program
	UDestroyShaderProgram(gProgramId);

	exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// GLFW: window creation
	// ---------------------
	*window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// GLEW: initialize
	// ----------------
	// Note: if using GLEW version 1.13 or earlier
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window) {
	static const float cameraSpeed = 2.5f;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessKeyboard(UP, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessKeyboard(DOWN, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pKeyPressed) {
		isPerspectiveView = !isPerspectiveView;
		pKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE)
		pKeyPressed = false;
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}



// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}


void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
	if (gFirstMouse) {
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos;
	gLastX = xpos;
	gLastY = ypos;
	gCamera.ProcessMouseMovement(xoffset, yoffset);
}

void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	gCamera.ProcessMouseScroll(yoffset);
}


void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT: {
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
							   break;

	case GLFW_MOUSE_BUTTON_MIDDLE: {
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
								 break;

	case GLFW_MOUSE_BUTTON_RIGHT: {
		if (action == GLFW_PRESS)
			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
								break;

	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}

// Functioned called to render a frame
void URender()
{
	// Enable z-depth
	glEnable(GL_DEPTH_TEST);
	// Clear the frame and z buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
	glm::mat4 rotation = glm::rotate(0.5f, glm::vec3(1.0, 1.0f, 1.0f));
	glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 model = translation * rotation * scale;
	glm::mat4 view = gCamera.GetViewMatrix();
	glm::mat4 worldView;
	glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	GLint modelLoc;
	GLint viewLoc;
	GLint projLoc;
	GLint objectColorLoc;
	// Creates a orthographic projection
	//projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	//projection = glm::perspective(glm::radians(60.0f), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	// Set the appropriate projection matrix based on the current view mode
	if (isPerspectiveView) {
		projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else {
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	}
	// Set the shader to be used
	glUseProgram(gProgramId);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gProgramId, "model");
	viewLoc = glGetUniformLocation(gProgramId, "view");
	projLoc = glGetUniformLocation(gProgramId, "projection");
	objectColorLoc = glGetUniformLocation(gProgramId, "uObjectColor");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	// Table Top

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(6.0f, 1.0f, 6.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 0.0f, 0.0f, 1.0f);


	//light 1
	gLightPosition = pointLightPositions[0];
	GLint keyColorLoc = glGetUniformLocation(gProgramId, "keyColor");
	GLint keyPositionLoc = glGetUniformLocation(gProgramId, "keyPos");

	glUniform3f(keyColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(keyPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

	//light 2
	gLightPosition = pointLightPositions[1];
	GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
	GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");

	glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

	//camera
	GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

	// Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
	glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);

	const glm::vec3 cameraPosition = cameraPos;
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexture3);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Ruler Body
	
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(2.0f, 1.0f,0.5f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 0.2f, 0.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexture1);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//light 1
	gLightPosition = pointLightPositions[0];
	keyColorLoc = glGetUniformLocation(gProgramId, "keyColor");
	keyPositionLoc = glGetUniformLocation(gProgramId, "keyPos");

	glUniform3f(keyColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(keyPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

	//light 2
	gLightPosition = pointLightPositions[1];
	lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
	lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");

	glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
	//camera
	viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

	// Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
	glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);

	//const glm::vec3 cameraPosition = cameraPos;
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexture3);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gPyramid4Mesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Box #1 on Top Of Ruler

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(4.0f, 0.25f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 0.2f, 0.15f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 0.5f, 0.5f, 0.0f, 1.0f);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexture1);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Box #2 on Top Of Ruler
	
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(4.0f, 0.25f, 0.1f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 0.2f, -0.15f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 0.5f, 0.5f, 0.0f, 1.0f);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexture1);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	//light 1
	gLightPosition = pointLightPositions[0];
	keyColorLoc = glGetUniformLocation(gProgramId, "keyColor");
	keyPositionLoc = glGetUniformLocation(gProgramId, "keyPos");

	glUniform3f(keyColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(keyPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

	//light 2
	gLightPosition = pointLightPositions[1];
	lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
	lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");

	glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
	//camera
	viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

	// Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
	glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);

	//const glm::vec3 cameraPosition = cameraPos;
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexture3);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gPyramid4Mesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// BOX CODE
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(1.5f, 1.5f, 1.5f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(0.0f), glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.75f, 1.0f, -1.5f));
	// Model matrix: transformations are applied right-to-left orderth
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 0.97f, 0.48f, 0.04f, 1.0f);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexture0);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	//
	//CREATE THE Sphere
	//
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gSphereMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(3.0f, 1.0, 1.5f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexture1);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// BOX CODE
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(1.0f, 0.5f, 1.0f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(0.0f), glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-2.0f, 0.5f, -1.5f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glProgramUniform4f(gProgramId, objectColorLoc, 0.97f, 0.48f, 0.04f, 1.0f);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexture0);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
	glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		//flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}

void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint &programId)
{
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader
	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);   // links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);    // Uses the shader program

	return true;
}


void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}