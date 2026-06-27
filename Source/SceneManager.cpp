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
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	bReturn = CreateGLTexture("../../Utilities/textures/black-metal-holes.jpg", "black-metal");
	bReturn = CreateGLTexture("../../Utilities/textures/black-rgb.jpg", "black-rgb");
	bReturn = CreateGLTexture("../../Utilities/textures/black-rubber.jpg", "black-rubber");
	bReturn = CreateGLTexture("../../Utilities/textures/black-fabric.jpg", "black-fabric");
	bReturn = CreateGLTexture("../../Utilities/textures/desk.jpg", "desk");
	bReturn = CreateGLTexture("../../Utilities/textures/desktop.jpg", "desktop");
	bReturn = CreateGLTexture("../../Utilities/textures/rgb.jpg", "rgb");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
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

	OBJECT_MATERIAL	metalMaterial;
	metalMaterial.ambientStrength = 0.05f;
	metalMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	metalMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	metalMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	metalMaterial.shininess = 64.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL	fabricMaterial;
	fabricMaterial.ambientStrength = 0.15f;
	fabricMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	fabricMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	fabricMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	fabricMaterial.shininess = 4.0;
	fabricMaterial.tag = "fabric";

	m_objectMaterials.push_back(fabricMaterial);

	OBJECT_MATERIAL deskMaterial;
	deskMaterial.ambientStrength = 0.08f;
	deskMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f);
	deskMaterial.diffuseColor = glm::vec3(0.45f, 0.45f, 0.45f);
	deskMaterial.specularColor = glm::vec3(0.08f, 0.08f, 0.08f);
	deskMaterial.shininess = 8.0f;
	deskMaterial.tag = "desk";

	m_objectMaterials.push_back(deskMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
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

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Main white light left
	m_pShaderManager->setVec3Value("lightSources[0].position", -10.0f, 20.0f, -6.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.08f, 0.08f, 0.08f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.15f, 0.15f, 0.15f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.2f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 12.0f);

	//Main white light right
	m_pShaderManager->setVec3Value("lightSources[1].position", 7.0f, 20.0f, -6.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[1]specularColor", 0.15f, 0.15f, 0.15f);
	m_pShaderManager->setFloatValue("lightSources[1]specularIntensity", 0.2f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);

	// RGB fan light
	m_pShaderManager->setVec3Value("lightSources[2].position", -10.0f, 4.0f, -0.5f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.05f, 0.02f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.85f, 0.25f, 0.75f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.85f, 0.25f, 0.75f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.4f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 4.0f);
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{

	// load the textures for the 3D scene
	LoadSceneTextures();

	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f);
	SetShaderTexture("desk");
	SetShaderMaterial("desk");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 9.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/*** Headset Stand												***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.0f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.0f, 0.1f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
	SetShaderTexture("black-rgb");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 4.6f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.0f, 2.4f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
	SetShaderTexture("black-metal");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(2.0f, 1.3f, 0.15f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -15.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(16.0f, 3.5f, 1.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawTorusMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.8f, 1.3f, 1.3f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -15.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(17.1f, 2.3f, 0.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees = 0.0f,
			YrotationDegrees,
			ZrotationDegrees = -15.0f,
			positionXYZ);

		//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.8f, 1.3f, 1.3f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -15.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(17.0f, 2.3f, 0.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees = 0.0f,
			YrotationDegrees,
			ZrotationDegrees = -15.0f,
			positionXYZ);

		//SetShaderColor(5.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("black-fabric");
		SetShaderMaterial("fabric");

		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.8f, 1.3f, 1.3f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -15.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(14.8f, 2.3f, 0.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees = 0.0f,
			YrotationDegrees,
			ZrotationDegrees = 15.0f,
			positionXYZ);

		//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.8f, 1.3f, 1.3f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -15.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(15.0f, 2.3f, 0.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees = 0.0f,
			YrotationDegrees,
			ZrotationDegrees = 15.0f,
			positionXYZ);

		//SetShaderColor(5.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("black-metal");
		SetShaderMaterial("fabric");

		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/*** Monitor													***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(2.5f, 2.5f, 2.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 0.1f, -4.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees = 0.0f,
			YrotationDegrees,
			ZrotationDegrees = 0.0f,
			positionXYZ);

		//SetShaderColor(5.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawPlaneMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.7f, 5.0f, 0.7f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 0.1f, -4.8f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees = 0.0f,
			YrotationDegrees,
			ZrotationDegrees = 0.0f,
			positionXYZ);

		//SetShaderColor(5.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(6.0f, 2.5f, 4.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 90.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 5.0f, -4.4f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(5.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawPlaneMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(5.8f, 2.3f, 3.8f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 90.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 5.0f, -4.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(5.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("desktop");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawPlaneMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/*** PC															***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(5.8f, 8.0f, 3.8f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-10.0f, 4.0f, -4.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(5.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -90.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-10.0f, 4.0f, -1.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(1.0f, 0.41f, 0.71f, 1);
		SetShaderTexture("rgb");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawConeMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -90.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-10.0f, 1.6f, -1.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(1.0f, 0.41f, 0.71f, 1);
		SetShaderTexture("rgb");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawConeMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -90.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-10.0f, 6.4f, -1.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(1.0f, 0.41f, 0.71f, 1);
		SetShaderTexture("rgb");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawConeMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -90.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-10.0f, 6.4f, -1.2f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawConeMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -90.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-10.0f, 4.0f, -1.2f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawConeMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -90.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-10.0f, 1.6f, -1.2f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawConeMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/*** Keyboard													***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 3.0f, 7.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -90.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 0.3f, 5.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.5f, 3.5f, 3.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = -90.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 0.9f, 5.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("rgb");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawPlaneMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 1.0f, 5.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-0.7f, 1.0f, 5.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.7f, 1.0f, 5.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 1.0f, 4.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 1.0f, 5.7f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-0.7f, 1.0f, 4.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.7f, 1.0f, 4.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-0.7f, 1.0f, 5.7f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.7f, 1.0f, 5.7f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.7f, 0.35f, 0.35f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 1.0f, 6.4f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/*** Mouse														***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.5f, 1.5f, 1.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(8.0f, 0.3f, 5.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		//SetShaderTexture("black-rubber");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawPlaneMesh();



		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.7f, 0.25f, 1.1f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(8.0f, 0.5f, 5.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
		SetShaderTexture("black-rubber");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.28f, 0.05f, 0.55f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = -10.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(8.3f, 0.7f, 4.4f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.0f, 0.0f, 1.0f, 1);
		SetShaderTexture("black-rubber");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.28f, 0.05f, 0.55f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = -10.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(7.7f, 0.7f, 4.4f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.0f, 0.0f, 1.0f, 1);
		SetShaderTexture("black-rubber");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/

		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.2f, 0.2f, 0.2f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(8.0f, 0.6f, 4.4f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.0f, 0.0f, 1.0f, 1);
		SetShaderTexture("black-rubber");
		//SetShaderMaterial("metal");

		// draw the mesh with transformation values
		m_basicMeshes->DrawTorusMesh();
}
