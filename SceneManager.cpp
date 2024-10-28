///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;
	bReturn = CreateGLTexture(
		"../../Utilities/textures/white_mug.jpg",
		"mug");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/ceramic.jpg",
		"mug2");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/countertop.jpg",
		"counter");
		bReturn = CreateGLTexture(
			"../../Utilities/textures/knife_handle.jpg",
			"cuttingBoard");
	
	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	// Material for the countertop
	OBJECT_MATERIAL counterMaterial;
	counterMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);  // Light gray ambient
	counterMaterial.ambientStrength = 0.3f; 
	counterMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);  
	counterMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); 
	counterMaterial.shininess = 32.0f;  
	counterMaterial.tag = "counter";
	m_objectMaterials.push_back(counterMaterial);

	// Material for the mug
	OBJECT_MATERIAL mugOuterMaterial;
	mugOuterMaterial.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);  // White ambient
	mugOuterMaterial.ambientStrength = 0.5f;  
	mugOuterMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	mugOuterMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f); 
	mugOuterMaterial.shininess = 8.0f; 
	mugOuterMaterial.tag = "mugOuter";
	m_objectMaterials.push_back(mugOuterMaterial);

	// Material for the handle 
	OBJECT_MATERIAL handleMaterial;
	handleMaterial.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);  
	handleMaterial.ambientStrength = 0.5f;  
	handleMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	handleMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	handleMaterial.shininess = 8.0f;
	handleMaterial.tag = "mugHandle";
	m_objectMaterials.push_back(handleMaterial);

	// Material for the cutting board
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.76f, 0.60f, 0.42f); // Brownish ambient color
	woodMaterial.ambientStrength = 0.5f; 
	woodMaterial.diffuseColor = glm::vec3(0.65f, 0.45f, 0.30f); 
	woodMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); 
	woodMaterial.shininess = 16.0f; 
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);


}

void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	// Activate lighting in the shader
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Light 1: Blue Spotlight
	m_pShaderManager->setVec3Value("lightSources[0].position", 7.5f, 20.0f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[0].direction", 0.0f, -1.0f, -0.5f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.05f, 0.05f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.1f, 0.1f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.2f, 0.2f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 350.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.9f);

	// Light 2: Directional white Light 
	m_pShaderManager->setVec3Value("lightSources[1].direction", -0.3f, -1.0f, -0.3f); 
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.1f, 0.1f); 
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.7f, 0.7f, 0.7f);   
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.5f, 0.5f, 0.5f); 

}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/

void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	DefineObjectMaterials();
	LoadSceneTextures();
	SetupSceneLights();


	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Draw the countertop
	DrawCountertop();

	// Draw the mug 
	DrawMug();

	// Draw the cutting board
	DrawCuttingBoard();

	// Draw the grapes
	DrawGrapes();

	// Draw the sausages
	DrawSausages();

	// Draw the tea box
	DrawTeaBox();

}

void SceneManager::DrawCountertop() {
	glm::vec3 scaleXYZ = glm::vec3(15.0f, 1.0f, 12.0f);
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ = glm::vec3(0.0f, 0.0f, 2.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	SetShaderTexture("counter");
	SetShaderMaterial("counter");
	m_basicMeshes->DrawPlaneMesh();
}

void SceneManager::DrawTeaBox() {
	glm::vec3 scaleXYZ = glm::vec3(4.0f, 2.0f, 2.0f); // Width, height, and depth
	float YrotationDegrees = -25.0f;
	glm::vec3 positionXYZ = glm::vec3(2.2f, 1.0f, 0.6f); // Adjust the X position to be left of the mug
	SetTransformations(scaleXYZ, 0.0f, YrotationDegrees, 0.0f, positionXYZ);

	SetShaderColor(0.8f, 0.7f, 0.5f, 1.0f); 

	m_basicMeshes->DrawBoxMesh();
}

// Function for 2 sausages
void SceneManager::DrawSausages() {

	glm::vec3 sausagePositions[2] = {
		glm::vec3(0.0f, 0.21f, 8.0f), // Sausage 1
		glm::vec3(0.7f, 0.21f, 7.0f)  // Sausage 2
	};

	glm::vec3 sausageScales[2] = {
		glm::vec3(0.1f, 0.1f, 1.8f),  // Scale for Sausage 1
		glm::vec3(0.1f, 0.1f, 1.8f)   // Scale for Sausage 2
	};

	float sausageRotations[2] = { 35.0f, 45.0f }; // Slightly different rotations to closer match picture

	// Loop through each sausage and draw it
	for (int i = 0; i < 2; i++) {
		glm::vec3 scaleXYZ = sausageScales[i];
		glm::vec3 positionXYZ = sausagePositions[i];
		float YrotationDegrees = sausageRotations[i];

		// Set transformations for the sausage
		SetTransformations(scaleXYZ, 0.0f, YrotationDegrees, 0.0f, positionXYZ);

		// Set shader color and material for sausages
		SetShaderColor(0.65f, 0.32f, 0.17f, 1.0f); // Brownish color for sausages

		m_basicMeshes->DrawCylinderMesh(false, true, true); // Draw a closed cylinder
	}
}

// Function to make 6 grapes
void SceneManager::DrawGrapes() {
	glm::vec3 grapePositions[6] = {
		glm::vec3(-3.0f, 0.4f, 7.5f), // Grape 1
		glm::vec3(-2.8f, 0.38f, 6.9f), // Grape 2
		glm::vec3(-2.6f, 0.40f, 7.3f), // Grape 3
		glm::vec3(-2.0f, 0.4f, 6.4f), // Grape 4
		glm::vec3(-2.6f, 0.40f, 7.8f), // Grape 5
		glm::vec3(-2.6f, 0.38f, 6.4f)  // Grape 6
	};

	float grapeSizes[6] = { 0.2f, 0.18f, 0.22f, 0.19f, 0.21f, 0.17f }; // Different sizes for each grape

	// Loop through each grape and draw it instead of seperate renderings for each
	for (int i = 0; i < 6; i++) {
		glm::vec3 scaleXYZ = glm::vec3(grapeSizes[i]);
		glm::vec3 positionXYZ = grapePositions[i];

		// Set transformations for the grape
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

		SetShaderColor(0.5f, 0.0f, 0.5f, 1.0f); // Purple color for grapes

		// Draw the grape using a sphere mesh
		m_basicMeshes->DrawSphereMesh();
	}
}

void SceneManager::DrawCuttingBoard() {
	glm::vec3 scaleXYZ = glm::vec3(6.0f, 0.2f, 3.0f); // Size
	glm::vec3 positionXYZ = glm::vec3(-1.0f, 0.1f, 7.5f); // Position 

	// Set transformations for the cutting board
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

	// Set a wooden texture color and material for the cutting board
	SetShaderColor(0.76f, 0.60f, 0.42f, 1.0f); // A brown color representing wood
	SetShaderTexture("cuttingBoard");
	SetShaderMaterial("wood");

	// Draw the box mesh for the cutting board
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::DrawMug() {
	// Draw the outer cylinder of the mug
	glm::vec3 scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);
	glm::vec3 positionXYZ = glm::vec3(5.5f, 0.0f, 3.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetTextureUVScale(5.0, 1.0);
	SetShaderTexture("mug");
	SetShaderMaterial("mugOuter");
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	// Draw the torus handle of the mug
	scaleXYZ = glm::vec3(0.7f, 0.7f, 0.2f);
	float YrotationDegrees = -15.0f;
	positionXYZ = glm::vec3(6.5f, 0.8f, 3.0f);

	SetTransformations(scaleXYZ, 0.0f, YrotationDegrees, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetTextureUVScale(2.0, 1.0);
	SetShaderTexture("mug");
	m_basicMeshes->DrawTorusMesh();

	// Draw the black outer rim of the mug
	scaleXYZ = glm::vec3(1.01f, 0.05f, 1.01f);
	positionXYZ = glm::vec3(5.5f, 2.0f, 3.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Draw the inner rim of the mug
	scaleXYZ = glm::vec3(0.99f, 0.05f, 0.99f);
	positionXYZ = glm::vec3(5.5f, 2.01f, 3.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Draw the tea tag box
	scaleXYZ = glm::vec3(0.4f, 0.6f, 0.1f);
	positionXYZ = glm::vec3(5.5f, 0.33f, 4.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Draw the string for the tea tag
	scaleXYZ = glm::vec3(0.02f, 1.75f, 0.02f);
	positionXYZ = glm::vec3(5.5f, 0.33f, 4.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.96f, 0.87f, 0.70f, 1.0f); // Beige color
	m_basicMeshes->DrawCylinderMesh();
}
