#include <cmath>
#include <cstdio>
#include <cstdlib>
#include<iostream>
// Headers abaixo s�o espec�ficos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Cria��o de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Cria��o de janelas do sistema operacional


// Headers da biblioteca GLM: cria��o de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Bibliotecas audio
#include <irrklang/include/irrKlang.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"


#define M_PI_2 1.57079632679489661923
#define M_PI   3.14159265358979323846

using namespace irrklang;
ISoundEngine* engine =createIrrKlangDevice();



// Estrutura que representa um modelo geom�trico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor l� o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};


// Declara��o de fun��es utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declara��o de v�rias fun��es utilizadas em main().  Essas est�o definidas
// logo ap�s a defini��o de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constr�i representa��o de um ObjModel como malha de tri�ngulos para renderiza��o
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso n�o existam.
void LoadShadersFromFiles(); // Carrega os shaders de v�rtice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Fun��o que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Fun��o utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Fun��o para debugging

// Declara��o de fun��es auxiliares para renderizar texto dentro da janela
// OpenGL. Estas fun��es est�o definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Fun��es abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informa��es do programa. Definidas ap�s main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Fun��es callback para comunica��o com o sistema operacional e intera��o do
// usu�rio. Veja mais coment�rios nas defini��es das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
//====================================================================DEFINI��ES QUE A CATA FEZ AGR=======================================================
//PARA CAMERA E MOVIMENTO

#define PI 3.14159265
glm::vec4 pos_char = glm::vec4(0.0f, 0.0f, 3.0f, 1.0f); // Posi��o do Personagem (character) e, consequentemente, da c�mera
glm::vec4 novoPos_char;
glm::vec4 front_vector = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f); // Vetor de dire��o de visualiza��o do personagem e, consequentemente, da c�mera
glm::vec4 up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // Vetor up da c�mera
void movimento();
float deltaTempo = 0.0f;
float ultimaFrame = 0.0f;
float tempoLastStep = 0.0f;
float tempoLastTransport = 0.0f;
float field_of_view = PI / 3.0f;
bool teclas[200]; // Vetor que Armazena se teclas foram pressionadas, usando-se o �ndice do vetor para identificar a tecla em si
void DrawLevel1(glm::mat4 view, glm::mat4 projection);
float velocidadeY = 0.0f;
bool firstMouse = true;
bool colisaoChao = true;
bool stopPassos = true;
bool testaColisao = false; // Inicializado com false, s� muda qnd tem movimenta��o
float g_CameraTheta = 0.0f; // �ngulo no plano ZX em rela��o ao eixo Z
float g_CameraPhi = 0.0f;   // �ngulo em rela��o ao eixo Y
float g_CameraDistance = 3.5f; // Dist�ncia da c�mera para a origem
bool g_UsePerspectiveProjection = true;
// Raz�o de propor��o da janela (largura/altura). Veja fun��o FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;
void validaMovimento();
void playGame();
int menu();
void optionsMenu();
int chooseOption =false;
bool enterPressed = false;
bool busyWKey = false;
bool busySKey = false;
bool busyJUMPKey = false;
bool pressW = false;
bool pressS = false;
bool pressSpace = false;
bool JUMPING = false;
double actualSecond;
void atualizaPulo();
int startJump = 0;
GLFWwindow* window;
void escreveMsgNaTela();
bool lostGame = false;
void fimJogo();
float g_AngleY = 0.0f;
void ScreenPosToWorldRay(
    int mouseX, int mouseY,             // Mouse position, in pixels, from bottom-left corner of the window
    int screenWidth, int screenHeight,  // Window size, in pixels
    glm::mat4 ViewMatrix,               // Camera position and orientation
    glm::mat4 ProjectionMatrix,         // Camera parameters (ratio, field of view, near and far planes)
    glm::vec3& out_origin,              // Ouput : Origin of the ray. /!\ Starts at the near plane, so if you want the ray to start at the camera's position instead, ignore this.
    glm::vec3& out_direction            // Ouput : Direction, in world space, of the ray that goes "through" the mouse.
);
bool TestRayOBBIntersection(
    glm::vec3 ray_origin,        // Ray origin, in world space
    glm::vec3 ray_direction,     // Ray direction (NOT target position!), in world space. Must be normalize()'d.
    glm::vec3 aabb_min,          // Minimum X,Y,Z coords of the mesh when not transformed at all.
    glm::vec3 aabb_max,          // Maximum X,Y,Z coords. Often aabb_min*-1 if your mesh is centered, but it's not always the case.
    glm::mat4 ModelMatrix,       // Transformation applied to the mesh (which will thus be also applied to its bounding box)
    float& intersection_distance // Output : distance between ray_origin and the intersection with the OBB
);
const GLFWvidmode* mode;
int widthScreen;
int heigthScreen;
glm::mat4 viewVar;
glm::mat4 projectionVar;
void checkNoteClick();
bool cliquei = false;
//====================================================================DEFINI��ES QUE A CATA FEZ AGR=======================================================
#define WALL  0
#define FLOOR 1
#define CEILING 2
#define CHAIR 3
#define COW 4
#define SPHERE 5
#define TABLE 6
#define BOMB 7
#define LAPTOP 8
#define BUTTON 9
#define DOOR 10
#define ARMCHAIR 11
#define AXES 12
#define COW2 13
#define SPHERE2 14
#define MIRA 15
#define KEY 16
#define BUNNY 17


#define SECONDS 300

// esquerda
glm::vec4 w;
glm::vec4 u;

void BuildLine();


// Definimos uma estrutura que armazenar� dados necess�rios para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    void*        first_index; // �ndice do primeiro v�rtice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    int          num_indices; // N�mero de �ndices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasteriza��o (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde est�o armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
    glm::mat4    model_matrix;
};
struct Player
{

    glm::vec4 bbox_min = glm::vec4(-0.5f, -1.0f, -0.5f,1.0f);
    glm::vec4 bbox_max = glm::vec4(0.5f,0.5f,0.5f,1.0f);

};
struct Plane
{
    glm::vec4 normal;
    float d; ///distance os plane to origin. It is on plane equation
};
Player player;
// Abaixo definimos vari�veis globais utilizadas em v�rias fun��es do c�digo.

// A cena virtual � uma lista de objetos nomeados, guardados em um dicion�rio
// (map).  Veja dentro da fun��o BuildTrianglesAndAddToVirtualScene() como que s�o inclu�dos
// objetos dentro da vari�vel g_VirtualScene, e veja na fun��o main() como
// estes s�o acessados.
std::map<std::string, SceneObject> g_VirtualScene;


// "g_LeftMouseButtonPressed = true" se o usu�rio est� com o bot�o esquerdo do mouse
// pressionado no momento atual. Veja fun��o MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // An�logo para bot�o direito do mouse
bool g_MiddleMouseButtonPressed = false; // An�logo para bot�o do meio do mouse

// Vari�vel que controla se o texto informativo ser� mostrado na tela.
bool g_ShowInfoText = true;

// Vari�veis que definem um programa de GPU (shaders). Veja fun��o LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;

// N�mero de texturas carregadas pela fun��o LoadTextureImage()
GLuint g_NumLoadedTextures = 0;


//vetores posicao
glm::vec4 primitives[4] = { glm::vec4(-1.0f,0.0f,1.0f,1.0f),
                            glm::vec4(1.0f,0.0f,1.0f,1.0f),
                            glm::vec4(1.0f,0.0f,-1.0f,1.0f),
                            glm::vec4(-1.0f,0.0f,-1.0f,1.0f)
                          };

Plane roomPlanes[6];
//bool checkCollisionCameraBBox(std::string objectName);
bool collisionCameraBBoxObjecBBox(std::string objectName);
bool checkCollisionAllRoomObjects();
std::vector<std::string> roomObjects;
bool isPointInsideBBOX(glm::vec3 point);

int seconds = SECONDS; //tempo de jogo
int game_Time = (int)glfwGetTime();
void updateTime();
using namespace std;
int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impress�o de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL vers�o 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Pedimos para utilizar o perfil "core", isto �, utilizaremos somente as
    // fun��es modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    //===================================================================================================================


    //==================================================================================================================
    mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
   // widthScreen =mode->width;
    widthScreen= 1024;
    //heigthScreen = mode->height;
    heigthScreen= 736;
    window = glfwCreateWindow(widthScreen, heigthScreen, "Scape Game Topper", NULL, NULL);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a fun��o de callback que ser� chamada sempre que o usu�rio
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os bot�es do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL dever�o renderizar nesta janela
    glfwMakeContextCurrent(window);




    // Carregamento de todas fun��es definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a fun��o de callback que ser� chamada sempre que a janela for
    // redimensionada, por consequ�ncia alterando o tamanho do "framebuffer"
    // (regi�o de mem�ria onde s�o armazenados os pixels da imagem).
//===============================================================================================================================================

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 1024,736);
   // FramebufferSizeCallback(window, mode->width, mode->height); // For�amos a chamada do callback acima, para definir g_ScreenRatio.*//
//======================================================================================================================================================
    // Imprimimos no terminal informa��es sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);


    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    // LoadTextureImage("../../data/tc-earth_daymap_surface.jpg");      // TextureImage0
    // LoadTextureImage("../../data/tc-earth_nightmap_citylights.gif"); // TextureImage1

    LoadTextureImage("../../data/wall.jpg"); // TextureImage0
    LoadTextureImage("../../data/floor.jpg"); // TextureImage1
    LoadTextureImage("../../data/ceiling.jpg"); // TextureImage2
    LoadTextureImage("../../data/tc-earth_daymap_surface.jpg"); // TextureImage3
    LoadTextureImage("../../data/hplaptop_d.jpg"); // TextureImage4
    LoadTextureImage("../../data/bomb_difuse_map.jpg"); // TextureImage5
    LoadTextureImage("../../data/bomb_normal_map.jpg"); // TextureImage6
    LoadTextureImage("../../data/bomb_specular_map.jpg"); // TextureImage7
    LoadTextureImage("../../data/tc-earth_nightmap_citylights.gif"); //TextureImage8
    LoadTextureImage("../../data/DoorUV.png"); //TextureImage9
     LoadTextureImage("../../data/keyB_tx.bmp"); //[
    // Constru�mos a representa��o de objetos geom�tricos atrav�s de malhas de tri�ngulos
    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);


    ObjModel bunnymodel("../../data/bunny.obj");
    ComputeNormals(&bunnymodel);
    BuildTrianglesAndAddToVirtualScene(&bunnymodel);

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel leftwallmodel("../../data/leftwall.obj");
    ComputeNormals(&leftwallmodel);
    BuildTrianglesAndAddToVirtualScene(&leftwallmodel);

    ObjModel rightwallmodel("../../data/rightwall.obj");
    ComputeNormals(&rightwallmodel);
    BuildTrianglesAndAddToVirtualScene(&rightwallmodel);

    ObjModel cowmodel("../../data/cow.obj");
    ComputeNormals(&cowmodel);
    BuildTrianglesAndAddToVirtualScene(&cowmodel);

    ObjModel coffeetable("../../data/coffee_table.obj","../../data/");
    ComputeNormals(&coffeetable);
    BuildTrianglesAndAddToVirtualScene(&coffeetable);

    ObjModel laptop("../../data/Laptop_High-Polay_HP_BI_2_obj.obj","../../data/");
    ComputeNormals(&laptop);
    BuildTrianglesAndAddToVirtualScene(&laptop);


    ObjModel bomb("../../data/bomb.obj","../../data/");
    ComputeNormals(&bomb);
    BuildTrianglesAndAddToVirtualScene(&bomb);

    ObjModel door("../../data/door.obj", "../../data/");
    ComputeNormals(&door);
    BuildTrianglesAndAddToVirtualScene(&door);


    ObjModel armchair("../../data/3dstylish-fav001.obj", "../../data/");
    ComputeNormals(&armchair);
    BuildTrianglesAndAddToVirtualScene(&armchair);


    ObjModel key("../../data/Key_B_02.obj", "../../data/");
    ComputeNormals(&key);
    BuildTrianglesAndAddToVirtualScene(&key);


    BuildLine();


    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o c�digo para renderiza��o de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slide 66 do documento "Aula_13_Clipping_and_Culling.pdf".
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 22 � 34 do documento "Aula_13_Clipping_and_Culling.pdf".
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    while(true)
    {
        g_UsePerspectiveProjection = false;
        int opMenu =menu();
        switch(opMenu)
        {

        case 0 :
            g_UsePerspectiveProjection = true;
            playGame();
            break;

        case 1:
            return 0;
            break;

        default:
            break;

        }
    }
    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

void escreveMsgNaTela() //charada
{
    char buffer[80];
    snprintf(buffer, 80, "Se eu digo tic, e a bomba diz tac, como tirar da porta o X?");
    TextRendering_PrintString(window,buffer, -1.0f, 1.0f, 1.0f);

}
///Coloca pra jogar
void playGame()
{
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    engine->setSoundVolume(0.7);



// Ficamos em loop, renderizando, at� que o usu�rio feche a janela
    while (!glfwWindowShouldClose(window))
    {

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de v�rtice e fragmentos).
        // os shaders de v�rtice e fragmentos).
        float tempoAtual = glfwGetTime();
        deltaTempo = tempoAtual - ultimaFrame;
        ultimaFrame = tempoAtual;


        // Computamos a matriz "View" utilizando os par�metros da c�mera para
        // definir o sistema de coordenadas da c�mera.  Veja slide 162 do
        // documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        viewVar = Matrix_Camera_View(pos_char, front_vector, up_vector);

        // Note que, no sistema de coordenadas da c�mera, os planos near e far
        // est�o no sentido negativo! Veja slides 180-183 do documento
        // "Aula_09_Projecoes.pdf".
        float nearplane = -0.1f;  // Posi��o do "near plane"
        float farplane = -500.0f; // Posi��o do "far plane"

        // Proje��o Perspectiva.
        projectionVar = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);


        validaMovimento();
        DrawLevel1(viewVar, projectionVar);
        movimento(); // Realiza os movimentos do Personagem de acordo com as teclas pressionadas
        if(lostGame == true) fimJogo();
        TextRendering_ShowFramesPerSecond(window);

        glfwSwapBuffers(window);

        glfwPollEvents();

    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    glfwSetWindowShouldClose(window, GL_FALSE);
}
//Mostra o menu e tal
int menu()
{


    float startPos = -0.5f;
    float startSize = 2.5f;
    float exitPos = -0.65f;
    float exitSize = 1.0f;


    int selectPos = 0;

    while(!enterPressed)
    {

        if(pressW && !busyWKey && selectPos > 0)
        {
            busyWKey = true;
            selectPos--;
        }
        if(pressS && !busySKey && selectPos < 2)
        {
            busySKey = true;
            selectPos++;
        }

        switch(selectPos)
        {
        case 0:
            startSize = 2.5f;
            exitSize = 1.0f;
            break;
        case 1:
            startSize = 1.0f;
            exitSize = 2.5f;
            break;
        default:
            break;
        }

        glfwMakeContextCurrent(window);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e tamb�m resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_id);
        float d = g_CameraDistance;
        float y = d*sin(g_CameraPhi);
        float z = d*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = d*cos(g_CameraPhi)*sin(g_CameraTheta);

        // Abaixo definimos as var�veis que efetivamente definem a c�mera virtual.
        // Veja slide 165 do documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::vec4 camera_position_c  = glm::vec4(x,y,z,1.0f); // Ponto "c", centro da c�mera
        glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a c�mera (look-at) estar� sempre olhando
        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a c�mera est� virada
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "c�u" (eito Y global)

        // Computamos a matriz "View" utilizando os par�metros da c�mera para
        // definir o sistema de coordenadas da c�mera.  Veja slide 169 do
        // documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Proje��o.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da c�mera, os planos near e far
        // est�o no sentido negativo! Veja slides 198-200 do documento
        // "Aula_09_Projecoes.pdf".
        float nearplane = -0.1f;  // Posi��o do "near plane"
        float farplane  = -100.0f; // Posi��o do "far plane"


            // Proje��o Ortogr�fica.
            // Para defini��o dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // veja slide 243 do documento "Aula_09_Projecoes.pdf".
            // Para simular um "zoom" ortogr�fico, computamos o valor de "t"
            // utilizando a vari�vel g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);


        glm::mat4 model = Matrix_Identity(); // Transforma��o identidade de modelagem
        // model = Matrix_Rotate_Y((float)glfwGetTime() * 0.1f);
        // Enviamos as matrizes "view" e "projection" para a placa de v�deo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas s�o
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, COW2);
        DrawVirtualObject("cow");

        TextRendering_PrintString(window, "Start", 0.0f, startPos, startSize);
        TextRendering_PrintString(window, "exit", 0.0f, exitPos, exitSize);

        glfwSwapBuffers(window);


        // Verificamos com o sistema operacional se houve alguma intera��o do
        // usu�rio (teclado, mouse, ...). Caso positivo, as fun��es de callback
        // definidas anteriormente usando glfwSet*Callback() ser�o chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    enterPressed = false;
    return selectPos;

}
Plane getPlaneEquation(glm::mat4 model)
{

    glm::vec4 origin = glm::vec4(0.0f,0.0f,0.0f,1.0f);
    glm::vec4 finalPoint[4] = {origin,origin,origin,origin};
    finalPoint[0] =  model * primitives[0];
    finalPoint[1] =  model * primitives[1];
    finalPoint[2] =  model * primitives[2];
    finalPoint[3] =  model * primitives[3];

    glm::vec4 diagonal1 = finalPoint[3] - finalPoint[1];
    glm::vec4 diagonal2 = finalPoint[2] - finalPoint[0];

    Plane newPlane;
    newPlane.normal = glm::normalize(crossproduct(diagonal1, diagonal2));

    newPlane.d = norm((model * origin) - origin);

    return newPlane;
}

void DrawLevel1(glm::mat4 view, glm::mat4 projection)
{
    glm::mat4 model = Matrix_Identity(); // Transforma��o identidade de modelagem

    //           R     G     B     A
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
    // e tamb�m resetamos todos os pixels do Z-buffer (depth buffer).
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    model = Matrix_Identity();
    glUseProgram(program_id);
    // Enviamos as matrizes "view" e "projection" para a placa de v�deo
    // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas s�o
    // efetivamente aplicadas em todos os pontos.
    glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

        glm::vec4 viewVector = normalize(front_vector);
   // std::cout << "x " << viewVector.x << std::endl;
  //  std::cout << "y " << viewVector.y << std::endl;
   // std::cout << "z " << viewVector.z << std::endl;
    //  std::cout << "phi " << g_CameraPhi << std::endl;

    model = Matrix_Translate(pos_char[0] + viewVector[0], pos_char[1] + viewVector[1], pos_char[2] + viewVector[2])
           //* Matrix_Rotate(g_CameraPhi + 3.14/2, u)
           //* Matrix_Rotate(3.14/2 + g_CameraTheta, up_vector)
           //* Matrix_Rotate(g_CameraPhi + 3.14/2, u)
          //* Matrix_Rotate(3.14/2 + g_CameraTheta, up_vector)
           * Matrix_Scale(0.008f, 0.008f, 0.008f)
           * Matrix_Rotate_X(-M_PI *  g_CameraPhi)
           * Matrix_Rotate_Z(-M_PI_2 *  g_CameraTheta);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, MIRA);
    DrawVirtualObject("sphere");

    //chao
    model = Matrix_Translate(0.0f,-1.0f,0.0f)
            * Matrix_Scale(4.0f, 1.0f, 4.0f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, FLOOR);
    DrawVirtualObject("plane");
    roomPlanes[0] = getPlaneEquation(model);

    /*
    std::cout << "chao" << std::endl;
    std::cout << roomPlanes[0].normal.x << std::endl;
    std::cout << roomPlanes[0].normal.y << std::endl;
    std::cout << roomPlanes[0].normal.z << std::endl;*/

    // teto
    model = Matrix_Translate(0.0f,1.0f,0.0f)
            * Matrix_Scale(4.0f, 1.0f, 4.0f)
            * Matrix_Rotate_X(M_PI);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, CEILING);
    DrawVirtualObject("plane");
    roomPlanes[1] = getPlaneEquation(model);
    /*
    std::cout << "teto" << std::endl;
    std::cout << roomPlanes[1].normal.x << std::endl;
    std::cout << roomPlanes[1].normal.y << std::endl;
    std::cout << roomPlanes[1].normal.z << std::endl;*/

    //esquerda
    model = Matrix_Translate(-4.0f,0.0f,0.0f)
            * Matrix_Scale(1.0f, 1.0f, 4.0f)
            * Matrix_Rotate_Z(-M_PI_2);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, WALL);
    DrawVirtualObject("leftwall");
    roomPlanes[2] = getPlaneEquation(model);
    roomPlanes[2].normal = -roomPlanes[2].normal;
    /*
    std::cout << "esquerda" << std::endl;
    std::cout << roomPlanes[2].normal.x << std::endl;
    std::cout << roomPlanes[2].normal.y << std::endl;
    std::cout << roomPlanes[2].normal.z << std::endl;*/

    //direita
    model = Matrix_Translate(4.0f,0.0f,0.0f)
            * Matrix_Scale(1.0f,1.0f,4.0f)
            * Matrix_Rotate_Z(M_PI_2);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, WALL);
    DrawVirtualObject("rightwall");
    roomPlanes[3] = getPlaneEquation(model);
    roomPlanes[3].normal = -roomPlanes[3].normal;
    /*
    std::cout << "direita" << std::endl;
    std::cout << roomPlanes[3].normal.x << std::endl;
    std::cout << roomPlanes[3].normal.y << std::endl;
    std::cout << roomPlanes[3].normal.z << std::endl;*/

    //fundo
    model = Matrix_Translate(0.0f,0.0f,-4.0f)
            * Matrix_Scale(4.0f,1.0f,1.0f)
            * Matrix_Rotate_X(M_PI_2);

    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, WALL);
    DrawVirtualObject("plane");
    roomPlanes[4] = getPlaneEquation(model);
    /*std::cout << "fundo" << std::endl;
    std::cout << roomPlanes[4].normal.x << std::endl;
    std::cout << roomPlanes[4].normal.y << std::endl;
    std::cout << roomPlanes[4].normal.z << std::endl;*/

    //frente
    model =   Matrix_Rotate_Z(M_PI)
              * Matrix_Translate(0.0f,0.0f,4.0f)
              * Matrix_Scale(4.0f,1.0f,1.0f)
              * Matrix_Rotate_X(-M_PI_2);

    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, WALL);
    DrawVirtualObject("plane");
    roomPlanes[5] = getPlaneEquation(model);
    /* std::cout << "frente" << std::endl;
     std::cout << roomPlanes[5].normal.x << std::endl;
     std::cout << roomPlanes[5].normal.y << std::endl;
     std::cout << roomPlanes[5].normal.z << std::endl;*/

    // COFFEE TABLE

    model = Matrix_Translate(+2.80f, -1.025f, -1.225f)
            * Matrix_Scale(0.003f, 0.002f, 0.002f)
            * Matrix_Rotate_Y(1 * PI / 8);

    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, TABLE); ///Oi, lau! Gostaria mtmt que funcionasse colocoar a textura do mapa nessa mesa. � estranho pq eu testei a testura no laptop e funciona, na msa � que n�o rola. Ve se tu consegue daii
    DrawVirtualObject("coffee_table"); //oi cata! saaad, tentarei!
    g_VirtualScene["coffee_table"].model_matrix = Matrix_Translate(+3.05f, -1.025f, -1.225f)
            * Matrix_Scale(0.002f, 0.002f, 0.002f);
    roomObjects.push_back("coffee_table");

    //laptop
    model = Matrix_Translate(+3.05f, -0.3f, -1.225f)
        * Matrix_Scale(0.2f, 0.2f, 0.2f)
          * Matrix_Rotate_Y(-PI);

	g_VirtualScene["HPPlane002"].model_matrix = model;
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, LAPTOP);
    DrawVirtualObject("HPPlane002");

   model = Matrix_Translate(+3.05f, -0.3f, -1.225f)
            * Matrix_Scale(0.2f, 0.2f, 0.2f)
            * Matrix_Rotate_Y(-PI);
   g_VirtualScene["HPPlane005_Plane"].model_matrix = model;
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
	glUniform1i(object_id_uniform, LAPTOP);
    DrawVirtualObject("HPPlane005_Plane");

    // bomb
    model = Matrix_Translate(-2.05f, -0.9f, -1.225f)
            * Matrix_Scale(0.02f, 0.02f, 0.02f)
            *Matrix_Rotate_Y(M_PI/4);

    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, BOMB);
    g_VirtualScene["Cylinder010"].model_matrix = Matrix_Translate(-2.05f, -0.9f, -1.225f)
            * Matrix_Scale(0.02f, 0.02f, 0.02f);
    DrawVirtualObject("Cylinder010"); /// Oi, lau! gostaria de colocar a bomba como cube ou sphere, mas n consegui. Ve se tu consegue, pq com plane ta mt p�ssimo
    roomObjects.push_back("Cylinder010"); //oi cata, nao tive tempo, mas depois eu tentarei


    model = Matrix_Translate(-2.5f, -0.67f, -4.2f)
            * Matrix_Scale(0.38f, 0.32f, 0.28f)
            * Matrix_Rotate_Y(-PI / 2);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, DOOR);
    DrawVirtualObject("Cube");

    model = Matrix_Translate(0.5f, -1.3f, -2.7f)
            * Matrix_Scale(0.04f, 0.04f, 0.04f);
    // * Matrix_Rotate_Y(1 * PI / 2);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, ARMCHAIR);
    DrawVirtualObject("Line02");
    g_VirtualScene["Line02"].model_matrix = model;
    roomObjects.push_back("Line02");

    model = Matrix_Translate(0.5f, -1.3f, -2.7f)
            * Matrix_Scale(0.04f, 0.04f, 0.04f);
    //* Matrix_Rotate_Y(1 * PI / 2);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, ARMCHAIR);
    DrawVirtualObject("cuadrado");
    g_VirtualScene["cuadrado"].model_matrix = model;
    roomObjects.push_back("cuadrado");


    glLineWidth(10.0f);
    model = Matrix_Translate(-2.5f,0.0f,-3.7f); //Matrix_Translate(0.0f,0.0f,0.0f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, AXES);
    DrawVirtualObject("axes");
    cliquei = true;
    if(cliquei == true){

        model = Matrix_Translate(-2.0f,0.0f,3.7f)
                * Matrix_Scale(0.08f, 0.08f, 0.08f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, KEY);
        g_VirtualScene["Key_B"].model_matrix = model;
        roomObjects.push_back("Key_B");
        DrawVirtualObject("Key_B");

       // model = Matrix_Translate(-2.0f, 0.0f, 3.7f)
        //        * Matrix_Scale(0.5f, 0.5f, 0.5f);
       // glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
       // glUniform1i(object_id_uniform, BUNNY);
       // DrawVirtualObject("bunny");
       // g_VirtualScene["bunny"].model_matrix = model;
      //  roomObjects.push_back("bunny");
        //if ()

    }


    updateTime();
    if (isPointInsideBBOX(glm::vec3(-2.5f,0.0f,-3.7f)) || seconds == 0 )
    {
        lostGame = true;
    }
    if(cliquei == true){
            //key

        escreveMsgNaTela();
    }




}

void updateTime()
{

    if((game_Time != (int)glfwGetTime()) && seconds > 0)
    {
        game_Time =(int)glfwGetTime();
        seconds--;
    }

    char buffer[44];
    sprintf ( buffer, "Voce tem    %d   segundos", seconds );
    TextRendering_PrintString(window,buffer, 0.60f, 0.90f, 1.0f);
}
void BuildLine()
{
    GLfloat model_coefficients[] =
    {
        //    X      Y     Z     W
        -1.0f,  1.0f,  0.0f, 1.0f, // posi��o do v�rtice 8
        1.0f,  1.0f,  0.0f, 1.0f, // posi��o do v�rtice 9
        1.0f,  -1.0f,  0.0f, 1.0f, // posi��o do v�rtice 9
        -1.0f,  -1.0f,  0.0f, 1.0f // posi��o do v�rtice 9

    };
    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(model_coefficients), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(model_coefficients), model_coefficients);
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint indices[] = {0, 2, 1, 3};

    // Criamos um terceiro objeto virtual (SceneObject) que se refere aos eixos XYZ.
    SceneObject axes;
    axes.name           = "axes";
    axes.first_index    = (void*)(0*sizeof(GLuint)); // Primeiro �ndice est� em indices[60]
    axes.num_indices    = 4; // �ltimo �ndice est� em indices[65]; total de 6 �ndices.
    axes.rendering_mode = GL_LINES; // �ndices correspondem ao tipo de rasteriza��o GL_LINES.
    axes.vertex_array_object_id = vertex_array_object_id;
    g_VirtualScene["axes"] = axes;

    // Criamos um buffer OpenGL para armazenar os �ndices acima
    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora � GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);

    // Alocamos mem�ria para o buffer.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), NULL, GL_STATIC_DRAW);

    // Copiamos os valores do array indices[] para dentro do buffer.
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    glBindVertexArray(0);
}
bool isPointInsideBBOX(glm::vec3 point)
{

    glm::vec4 playerbbox_max = glm::vec4(novoPos_char.x + 0.5f, novoPos_char.y + 0.5f, novoPos_char.z + 0.5f, 1.0f);
    glm::vec4 playerbbox_min = glm::vec4(novoPos_char.x - 0.5f, novoPos_char.y - 1.0f, novoPos_char.z - 0.5f, 1.0f);


    return (point.x >= playerbbox_min.x && point.x <= playerbbox_max.x) &&
           (point.y >= playerbbox_min.y && point.y <= playerbbox_max.y) &&
           (point.z >= playerbbox_min.z && point.z <= playerbbox_max.z);

}


bool collisionCameraBBoxObjecBBox(std::string objectName)
{


    glm::vec4 objBBox_max = g_VirtualScene[objectName].model_matrix * glm::vec4(g_VirtualScene[objectName].bbox_max.x,
                            g_VirtualScene[objectName].bbox_max.y,
                            g_VirtualScene[objectName].bbox_max.z,
                            1.0f);
    glm::vec4 objBBox_min = g_VirtualScene[objectName].model_matrix * glm::vec4(g_VirtualScene[objectName].bbox_min.x,
                            g_VirtualScene[objectName].bbox_min.y,
                            g_VirtualScene[objectName].bbox_min.z,
                            1.0f);

    /*    std::cout << "PLAYER Xmin = " << player.bbox_min.x << std::endl;
        std::cout << "PLAYER Ymin = " << player.bbox_min.y << std::endl;
        std::cout << "PLAYER Zmin = " << player.bbox_min.z << std::endl;

        std::cout << "PLAYER Xmax = " << player.bbox_max.x << std::endl;
        std::cout << "PLAYER Ymax = " << player.bbox_max.y << std::endl;
        std::cout << "PLAYER Zmax = " << player.bbox_max.z << std::endl;

        std::cout << "OBJ minX = " << objBBox_min.x << std::endl;
        std::cout << "OBJ minY = " << objBBox_min.y << std::endl;
        std::cout << "OBJ minZ = " << objBBox_min.z << std::endl;

        std::cout << "OBJ maxX = " << objBBox_max.x << std::endl;
        std::cout << "OBJ maxY = " << objBBox_max.y << std::endl;
        std::cout << "OBJ maxZ = " << objBBox_max.z << std::endl;*/


    glm::vec4 playerbbox_max = glm::vec4(novoPos_char.x + 0.5f, novoPos_char.y + 0.5f, novoPos_char.z + 0.5f, 1.0f);
    glm::vec4 playerbbox_min = glm::vec4(novoPos_char.x - 0.5f, novoPos_char.y - 1.0f, novoPos_char.z - 0.5f, 1.0f);

    bool playerCollided = (playerbbox_max.x > objBBox_min.x && playerbbox_min.x < objBBox_max.x) &&
                            (playerbbox_max.y > objBBox_min.y && playerbbox_min.y < objBBox_max.y) &&
                            (playerbbox_max.z > objBBox_min.z && playerbbox_min.z < objBBox_max.z);

    if (objectName.compare("bunny") == 0 && playerCollided) {
        return false;
    }
    else if (objectName.compare("Key_B") == 0 && playerCollided){
        fimJogo();
        return true;
    }else {
        return playerCollided;
    }


}
bool checkCollisionAllRoomObjects()
{
    for(std::string objectName : roomObjects)
    {
        if (collisionCameraBBoxObjecBBox(objectName))
        {
            return true;
        }
    }
    return false;
}


void movimento()
{
    glm::vec4 dir_movimento = normalize(glm::vec4(front_vector.x, 0.0f, front_vector.z, 0.0f));
    float velocidade = 4.0f * deltaTempo;
    tempoLastStep += deltaTempo;
    tempoLastTransport += deltaTempo;
    bool step = false;

    if (teclas[GLFW_KEY_W])
    {
        novoPos_char += velocidade * dir_movimento;
        step = true;
    }

    if (teclas[GLFW_KEY_S])
    {
        novoPos_char -= velocidade * dir_movimento;
        step = true;
    }

    if (teclas[GLFW_KEY_A])
    {
        novoPos_char -= normalize(crossproduct(dir_movimento, up_vector)) * velocidade;
        step = true;
    }

    if (teclas[GLFW_KEY_D])
    {
        novoPos_char += normalize(crossproduct(dir_movimento, up_vector)) * velocidade;
        step = true;
    }
    // Movimento Vertical
    if (colisaoChao)
    {
        if (step && testaColisao)
            if (tempoLastStep > 0.5f)
            {
                if(stopPassos ==false) engine->play2D("../../data/audio/Footstep.wav");
                tempoLastStep = 0;
            }

        if (teclas[GLFW_KEY_SPACE])
        {
            if(stopPassos ==false) engine->play2D("../../data/audio/jump.wav");
            velocidadeY = 1.0f;
        }
    }
    novoPos_char.y += velocidadeY * deltaTempo;
    velocidadeY -= 1.0f * deltaTempo;
}

bool collided()
{
    for (int cont =2; cont <= 5; cont ++)
    {
        float A = roomPlanes[cont].normal.x;
        float B = roomPlanes[cont].normal.y;
        float C = roomPlanes[cont].normal.z;
        float D = roomPlanes[cont].d;
        float x = novoPos_char.x;
        float y = novoPos_char.y;
        float z = novoPos_char.z;

        if (fabs(x*A + y*B + z*C + D) < 0.3)
        {
            std::cout << " colidiu" << std::endl;
                printf("%d", widthScreen);
            std::cout << " conta = " << (x*A + y*B + z*C + D) << std::endl;
            std::cout << "cont = " << cont << std::endl;
            std::cout << "A = " << A << std::endl;
            std::cout << "B = " << B << std::endl;
            std::cout << "C = " << C << std::endl;
            std::cout << "D = " << D << std::endl;
            std::cout << "x = " << x << std::endl;
            std::cout << "y = " << y << std::endl;
            std::cout << "z = " << z << std::endl;
            return true;
        }
    }
    return false;
}
void updateBBox()
{

    player.bbox_max = glm::vec4(pos_char.x + 0.5f, pos_char.y + 0.5f, pos_char.z + 0.5f, 1.0f);
    player.bbox_min = glm::vec4(pos_char.x - 0.5f, pos_char.y - 1.0f, pos_char.z - 0.5f, 1.0f);

}
void validaMovimento()
{
    testaColisao = true;
    colisaoChao = false;

    if (!collided() && !checkCollisionAllRoomObjects())
    {
        stopPassos = false;
        pos_char.x = novoPos_char.x;
        pos_char.z = novoPos_char.z;
        updateBBox();
    }
    else
    {
        stopPassos = true;
        std::cout << " colidiu com algum obj" << std::endl;
        novoPos_char.x = pos_char.x;
        novoPos_char.z = pos_char.z;
    }
    /// Teste com o plano chao( y <=0 � se a pessoa ta no chao dnv, antes disso ela ta com y maior q zero pq ta no ar)
    ///O movimento vertical � meio foda pq ele usa aquele tro�o de anima��o que o sor fez na aula, lembra?
    ///al�m disso essa fun��o aqui validamovimento() testa se houve colisao, pq tipo, quando tiver colisao com o chao oo personagem para de cair sabe
    ///isso n faria sentido ficar na de cima a movimento() que s� soma constantes ao movimento. Na movimento() fica, por exemplo, a soma daqueilo de velocidade que vimos na aula
    ///que � computado nas linhas que tu tinha comentado.
    ///ela precisa ficar fora do if pq naquele caso ali ela n�o vai ter colisao com o ch�o e precisa entrar naquelas duas linhas, pq elas � que calculam a velocidade como vimos na aula
    ///e preciso da velo sendo calculada all time. Sei q dps de ler tudo isso tu ainda ta mais confusa, pessoalmente ter�a eu te falo

    ///OBS: se quisermos deixar o personagem subir nas mesas e pa d� p fazer facil, mas como ainda os objetos n tem colisao n d� p fazer
    if (novoPos_char.y <= 0)
    {
        colisaoChao = true;

    }
    if (colisaoChao)
    {
        novoPos_char.y = pos_char.y;
        velocidadeY = 0.0f;
    }
    else
        pos_char.y = novoPos_char.y;

}

bool checkCollisionCameraBBox(glm::vec4 bbox_min, glm::vec4 bbox_max)
{

    std::cout << "Xmin = " << bbox_min.x << std::endl;
    std::cout << "Ymin = " << bbox_min.y << std::endl;
    std::cout << "Zmin = " << bbox_min.z << std::endl;

    std::cout << "Xmax = " << bbox_max.x << std::endl;
    std::cout << "Ymax = " << bbox_max.y << std::endl;
    std::cout << "Zmax = " << bbox_max.z << std::endl;

    std::cout << "Xcamera = " << pos_char.x << std::endl;
    std::cout << "Ycamera = " << pos_char.y << std::endl;
    std::cout << "Zcamera = " << pos_char.z << std::endl;

    return (pos_char.x >= bbox_min.x && pos_char.x <= bbox_max.x) &&
           (pos_char.y >= bbox_min.y && pos_char.y <= bbox_max.y) &&
           (pos_char.z >= bbox_min.z && pos_char.z <= bbox_max.z);

}


// Fun��o que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slide 160 do documento "Aula_20_e_21_Mapeamento_de_Texturas.pdf"
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Par�metros de amostragem da textura. Falaremos sobre eles em uma pr�xima aula.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Fun��o que desenha um objeto armazenado em g_VirtualScene. Veja defini��o
// dos objetos na fun��o BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // v�rtices apontados pelo VAO criado pela fun��o BuildTrianglesAndAddToVirtualScene(). Veja
    // coment�rios detalhados dentro da defini��o de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as vari�veis "bbox_min" e "bbox_max" do fragment shader
    // com os par�metros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os v�rtices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a defini��o de
    // g_VirtualScene[""] dentro da fun��o BuildTrianglesAndAddToVirtualScene(), e veja
    // a documenta��o da fun��o glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene[object_name].first_index
    );

    // "Desligamos" o VAO, evitando assim que opera��es posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Fun��o que carrega os shaders de v�rtices e de fragmentos que ser�o
// utilizados para renderiza��o. Veja slide 217 e 219 do documento
// "Aula_03_Rendering_Pipeline_Grafico.pdf".
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" est�o fixados, sendo que assumimos a exist�ncia
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endere�o das vari�veis definidas dentro do Vertex Shader.
    // Utilizaremos estas vari�veis para enviar dados para a placa de v�deo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Vari�vel da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Vari�vel da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Vari�vel da matriz "projection" em shader_vertex.glsl
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); // Vari�vel "object_id" em shader_fragment.glsl
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");

    // Vari�veis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);

    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage8"), 8);
        glUniform1i(glGetUniformLocation(program_id, "TextureImage9"), 9);
              glUniform1i(glGetUniformLocation(program_id, "TextureImage10"), 10);
    glUseProgram(0);
}

// Fun��o que computa as normais de um ObjModel, caso elas n�o tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRI�NGULOS.
    // Segundo, computamos as normais dos V�RTICES atrav�s do m�todo proposto
    // por Gourad, onde a normal de cada v�rtice vai ser a m�dia das normais de
    // todas as faces que compartilham este v�rtice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

void fimJogo()
{
//TODO: testar dai se o player acabou o jogo perdendo ou n

    float startPos = -0.5f;
    float startSize = 2.5f;
    float exitPos = -0.65f;
    float exitSize = 1.0f;


    int selectPos = 0;

    while(!enterPressed)
    {

        if(pressW && !busyWKey && selectPos > 0)
        {
            busyWKey = true;
            selectPos--;
        }
        if(pressS && !busySKey && selectPos < 2)
        {
            busySKey = true;
            selectPos++;
        }

        switch(selectPos)
        {
        case 0:
            startSize = 2.5f;
            exitSize = 1.0f;
            break;
        case 1:
            startSize = 1.0f;
            exitSize = 2.5f;
            break;
        default:
            break;
        }

        glfwMakeContextCurrent(window);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e tamb�m resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_id);
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        // Abaixo definimos as var�veis que efetivamente definem a c�mera virtual.
        // Veja slide 165 do documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::vec4 camera_position_c  = glm::vec4(x,y,z,1.0f); // Ponto "c", centro da c�mera
        glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a c�mera (look-at) estar� sempre olhando
        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a c�mera est� virada
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "c�u" (eito Y global)

        // Computamos a matriz "View" utilizando os par�metros da c�mera para
        // definir o sistema de coordenadas da c�mera.  Veja slide 169 do
        // documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Proje��o.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da c�mera, os planos near e far
        // est�o no sentido negativo! Veja slides 198-200 do documento
        // "Aula_09_Projecoes.pdf".
        float nearplane = -0.1f;  // Posi��o do "near plane"
        float farplane  = -100.0f; // Posi��o do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Proje��o Perspectiva.
            // Para defini��o do field of view (FOV), veja slide 234 do
            // documento "Aula_09_Projecoes.pdf".
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Proje��o Ortogr�fica.
            // Para defini��o dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // veja slide 243 do documento "Aula_09_Projecoes.pdf".
            // Para simular um "zoom" ortogr�fico, computamos o valor de "t"
            // utilizando a vari�vel g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }


        glm::mat4 model = Matrix_Identity()* Matrix_Scale(0.02f, 0.02f, 0.02f); // Transforma��o identidade de modelagem
        // model = Matrix_Rotate_Y((float)glfwGetTime() * 0.1f);
        // Enviamos as matrizes "view" e "projection" para a placa de v�deo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas s�o
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, BOMB);
        DrawVirtualObject("Cylinder010");


        model = Matrix_Translate(-3.5f, -0.7f,-3.0f)
                * Matrix_Scale(0.8f, 0.8f, 0.8f)
                * Matrix_Rotate_Z(0.6f)
                * Matrix_Rotate_X(0.2f)
                * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, SPHERE2);
        DrawVirtualObject("sphere");

        if(lostGame == true) TextRendering_PrintString(window, "Tic tac, a bomba explodiu! Voc� saiu do mundo :(", 0.0f, startPos, startSize);
        else TextRendering_PrintString(window, "Parab�ns! Voc� continua nesse mundo :)", 0.0f, startPos, startSize);
        TextRendering_PrintString(window, "Sair", 0.0f, exitPos, exitSize);

        glfwSwapBuffers(window);


        // Verificamos com o sistema operacional se houve alguma intera��o do
        // usu�rio (teclado, mouse, ...). Caso positivo, as fun��es de callback
        // definidas anteriormente usando glfwSet*Callback() ser�o chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    enterPressed = false;
    if(selectPos == 1)
    {
        // Finalizamos o uso dos recursos do sistema operacional
        glfwTerminate();

        // Fim do programa
        glfwSetWindowShouldClose(window, GL_FALSE);
    }

}

// Constr�i tri�ngulos para futura renderiza��o a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();

        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o c�digo da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel �
                // comparando se o �ndice retornado � -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = (void*)first_index; // Primeiro �ndice
        theobject.num_indices    = last_index - first_index + 1; // N�mero de indices
        theobject.rendering_mode = GL_TRIANGLES;       // �ndices correspondem ao tipo de rasteriza��o GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        glm::vec4 bbox_min_vec4 = glm::vec4(bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
        glm::vec4 bbox_max_vec4 = glm::vec4(bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

        theobject.bbox_min = bbox_min;   //_vec4;
        theobject.bbox_max = bbox_max;    //_vec4;
        theobject.model_matrix = Matrix_Identity();

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!

    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja defini��o de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // ser� aplicado nos v�rtices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja defini��o de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // ser� aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Fun��o auxilar, utilizada pelas duas fun��es acima. Carrega c�digo de GPU de
// um arquivo GLSL e faz sua compila��o.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela vari�vel "filename"
    // e colocamos seu conte�do em mem�ria, apontado pela vari�vel
    // "shader_string".
    std::ifstream file;
    try
    {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    }
    catch ( std::exception& e )
    {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o c�digo do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o c�digo do shader GLSL (em tempo de execu��o)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compila��o
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos mem�ria para guardar o log de compila��o.
    // A chamada "new" em C++ � equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compila��o
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ � equivalente ao "free()" do C
    delete [] log;
}

// Esta fun��o cria um programa de GPU, o qual cont�m obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Defini��o dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos mem�ria para guardar o log de compila��o.
        // A chamada "new" em C++ � equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ � equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para dele��o ap�s serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Defini��o da fun��o que ser� chamada sempre que a janela do sistema
// operacional for redimensionada, por consequ�ncia alterando o tamanho do
// "framebuffer" (regi�o de mem�ria onde s�o armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda regi�o do framebuffer. A
    // fun��o "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa � a opera��o de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula (slide 183 do
    // documento "Aula_03_Rendering_Pipeline_Grafico.pdf").
    glViewport(0, 0, width, height);
    // Atualizamos tamb�m a raz�o que define a propor��o da janela (largura /
    // altura), a qual ser� utilizada na defini��o das matrizes de proje��o,
    // tal que n�o ocorra distor��es durante o processo de "Screen Mapping"
    // acima, quando NDC � mapeado para coordenadas de pixels. Veja slide 199
    // do documento "Aula_09_Projecoes.pdf".
    //
    // O cast para float � necess�rio pois n�meros inteiros s�o arredondados ao
    // ser divididos!
    g_ScreenRatio = (float)width / height;
}

// Vari�veis globais que armazenam a �ltima posi��o do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Fun��o callback chamada sempre que o usu�rio aperta algum dos bot�es do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usu�rio pressionou o bot�o esquerdo do mouse, guardamos a
        // posi��o atual do cursor nas vari�veis g_LastCursorPosX e
        // g_LastCursorPosY.  Tamb�m, setamos a vari�vel
        // g_LeftMouseButtonPressed como true, para saber que o usu�rio est�
        // com o bot�o esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usu�rio soltar o bot�o esquerdo do mouse, atualizamos a
        // vari�vel abaixo para false.
		g_LeftMouseButtonPressed = false;
        checkNoteClick();
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usu�rio pressionou o bot�o esquerdo do mouse, guardamos a
        // posi��o atual do cursor nas vari�veis g_LastCursorPosX e
        // g_LastCursorPosY.  Tamb�m, setamos a vari�vel
        // g_RightMouseButtonPressed como true, para saber que o usu�rio est�
        // com o bot�o esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usu�rio soltar o bot�o esquerdo do mouse, atualizamos a
        // vari�vel abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usu�rio pressionou o bot�o esquerdo do mouse, guardamos a
        // posi��o atual do cursor nas vari�veis g_LastCursorPosX e
        // g_LastCursorPosY.  Tamb�m, setamos a vari�vel
        // g_MiddleMouseButtonPressed como true, para saber que o usu�rio est�
        // com o bot�o esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usu�rio soltar o bot�o esquerdo do mouse, atualizamos a
        // vari�vel abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
    //deixa essa funcao ai, lau, pls. q eu vou usar ela p ver se a pessoa clicou no note
    // if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    //{
    // double xpos, ypos;
    //getting cursor position
    // glfwGetCursorPos(window, &xpos, &ypos);
    //cout << "Cursor Position at (" << xpos << " : " << ypos << endl;
    //}
}

// Fun��o callback chamada sempre que o usu�rio movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{

    if(firstMouse)
    {
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
        firstMouse = false;
    }


    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Atualizamos par�metros da c�mera com os deslocamentos

    g_CameraTheta -= 0.01f*dx;
    g_CameraPhi += 0.01f*dy;

    // Em coordenadas esf�ricas, o �ngulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f / 2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    // Atualizamos as vari�veis globais para armazenar a posi��o atual do
    // cursor como sendo a �ltima posi��o conhecida do cursor.



    glm::vec4 frente;
    frente.x = -cos(g_CameraTheta)*cos(g_CameraPhi);
    frente.y = -sin(g_CameraPhi);
    frente.z = sin(g_CameraTheta)*cos(g_CameraPhi);
    frente.w = 0.0f;
    front_vector = normalize(frente);

    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;


}


// Fun��o callback chamada sempre que o usu�rio movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a dist�ncia da c�mera para a origem utilizando a
    // movimenta��o da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma c�mera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela est� olhando, pois isto gera problemas de divis�o por zero na
    // defini��o do sistema de coordenadas da c�mera. Isto �, a vari�vel abaixo
    // nunca pode ser zero. Vers�es anteriores deste c�digo possu�am este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Defini��o da fun��o que ser� chamada sempre que o usu�rio pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{

    // Se o usu�rio pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if ( (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) && action == GLFW_PRESS)
    {
        enterPressed = true;
    }
    if ( (key == GLFW_KEY_W  ) && action == GLFW_PRESS)
    {
        pressW = true;
    }
    if  (key == GLFW_KEY_S)
    {
        pressS = true;
    }
    // Se o usu�rio apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usu�rio apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout, "Shaders recarregados!\n");
        fflush(stdout);
    }
    //release na key
    if ( (key == GLFW_KEY_W) && action == GLFW_RELEASE)
    {
        pressW = false;
        busyWKey = false;
    }

    if ((key == GLFW_KEY_S ) && action == GLFW_RELEASE)
    {
        pressS = false;
        busySKey = false;
    }

    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);


    if (key >= 0 && key < 200)
    {
        if (action == GLFW_PRESS)
            teclas[key] = true;
        else if (action == GLFW_RELEASE)
            teclas[key] = false;
    }

}

// Definimos o callback para impress�o de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta fun��o recebe um v�rtice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transforma��es.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     World", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     Camera", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                   NDC", -1.0f, 1.0f-13*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-14*pad, 1.0f);
}

// Escrevemos na tela qual matriz de proje��o est� sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o n�mero de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Vari�veis est�ticas (static) mant�m seus valores entre chamadas
    // subsequentes da fun��o!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o n�mero de segundos que passou desde a execu��o do programa
    float seconds = (float)glfwGetTime();

    // N�mero de segundos desde o �ltimo c�lculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Fun��o para debugging: imprime no terminal todas informa��es de um modelo
// geom�trico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
    const tinyobj::attrib_t                & attrib    = model->attrib;
    const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
    const std::vector<tinyobj::material_t> & materials = model->materials;

    printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
    printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
    printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
    printf("# of shapes    : %d\n", (int)shapes.size());
    printf("# of materials : %d\n", (int)materials.size());

    for (size_t v = 0; v < attrib.vertices.size() / 3; v++)
    {
        printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.vertices[3 * v + 0]),
               static_cast<const double>(attrib.vertices[3 * v + 1]),
               static_cast<const double>(attrib.vertices[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.normals.size() / 3; v++)
    {
        printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.normals[3 * v + 0]),
               static_cast<const double>(attrib.normals[3 * v + 1]),
               static_cast<const double>(attrib.normals[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.texcoords.size() / 2; v++)
    {
        printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.texcoords[2 * v + 0]),
               static_cast<const double>(attrib.texcoords[2 * v + 1]));
    }

    // For each shape
    for (size_t i = 0; i < shapes.size(); i++)
    {
        printf("shape[%ld].name = %s\n", static_cast<long>(i),
               shapes[i].name.c_str());
        printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.indices.size()));

        size_t index_offset = 0;

        assert(shapes[i].mesh.num_face_vertices.size() ==
               shapes[i].mesh.material_ids.size());

        printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

        // For each face
        for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++)
        {
            size_t fnum = shapes[i].mesh.num_face_vertices[f];

            printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
                   static_cast<unsigned long>(fnum));

            // For each vertex in the face
            for (size_t v = 0; v < fnum; v++)
            {
                tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
                printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
                       static_cast<long>(v), idx.vertex_index, idx.normal_index,
                       idx.texcoord_index);
            }

            printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
                   shapes[i].mesh.material_ids[f]);

            index_offset += fnum;
        }

        printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.tags.size()));
        for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++)
        {
            printf("  tag[%ld] = %s ", static_cast<long>(t),
                   shapes[i].mesh.tags[t].name.c_str());
            printf(" ints: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j)
            {
                printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
                if (j < (shapes[i].mesh.tags[t].intValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");

            printf(" floats: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j)
            {
                printf("%f", static_cast<const double>(
                           shapes[i].mesh.tags[t].floatValues[j]));
                if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");

            printf(" strings: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j)
            {
                printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
                if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");
            printf("\n");
        }
    }

    for (size_t i = 0; i < materials.size(); i++)
    {
        printf("material[%ld].name = %s\n", static_cast<long>(i),
               materials[i].name.c_str());
        printf("  material.Ka = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].ambient[0]),
               static_cast<const double>(materials[i].ambient[1]),
               static_cast<const double>(materials[i].ambient[2]));
        printf("  material.Kd = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].diffuse[0]),
               static_cast<const double>(materials[i].diffuse[1]),
               static_cast<const double>(materials[i].diffuse[2]));
        printf("  material.Ks = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].specular[0]),
               static_cast<const double>(materials[i].specular[1]),
               static_cast<const double>(materials[i].specular[2]));
        printf("  material.Tr = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].transmittance[0]),
               static_cast<const double>(materials[i].transmittance[1]),
               static_cast<const double>(materials[i].transmittance[2]));
        printf("  material.Ke = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].emission[0]),
               static_cast<const double>(materials[i].emission[1]),
               static_cast<const double>(materials[i].emission[2]));
        printf("  material.Ns = %f\n",
               static_cast<const double>(materials[i].shininess));
        printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
        printf("  material.dissolve = %f\n",
               static_cast<const double>(materials[i].dissolve));
        printf("  material.illum = %d\n", materials[i].illum);
        printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
        printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
        printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
        printf("  material.map_Ns = %s\n",
               materials[i].specular_highlight_texname.c_str());
        printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
        printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
        printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
        printf("  <<PBR>>\n");
        printf("  material.Pr     = %f\n", materials[i].roughness);
        printf("  material.Pm     = %f\n", materials[i].metallic);
        printf("  material.Ps     = %f\n", materials[i].sheen);
        printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
        printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
        printf("  material.aniso  = %f\n", materials[i].anisotropy);
        printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
        printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
        printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
        printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
        printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
        printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
        std::map<std::string, std::string>::const_iterator it(
            materials[i].unknown_parameter.begin());
        std::map<std::string, std::string>::const_iterator itEnd(
            materials[i].unknown_parameter.end());

        for (; it != itEnd; it++)
        {
            printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
        }
        printf("\n");
    }
}

void ScreenPosToWorldRay(
    int mouseX, int mouseY,             // Mouse position, in pixels, from bottom-left corner of the window
    int screenWidth, int screenHeight,  // Window size, in pixels
    glm::mat4 ViewMatrix,               // Camera position and orientation
    glm::mat4 ProjectionMatrix,         // Camera parameters (ratio, field of view, near and far planes)
    glm::vec3& out_origin,              // Ouput : Origin of the ray. /!\ Starts at the near plane, so if you want the ray to start at the camera's position instead, ignore this.
    glm::vec3& out_direction            // Ouput : Direction, in world space, of the ray that goes "through" the mouse.
)
{

    // The ray Start and End positions, in Normalized Device Coordinates (Have you read Tutorial 4 ?)
    glm::vec4 lRayStart_NDC(
        ((float)mouseX/(float)screenWidth  - 0.5f) * 2.0f, // [0,1024] -> [-1,1]
        ((float)mouseY/(float)screenHeight - 0.5f) * 2.0f, // [0, 768] -> [-1,1]
        -1.0, // The near plane maps to Z=-1 in Normalized Device Coordinates
        1.0f
    );
    glm::vec4 lRayEnd_NDC(
        ((float)mouseX/(float)screenWidth  - 0.5f) * 2.0f,
        ((float)mouseY/(float)screenHeight - 0.5f) * 2.0f,
        0.0,
        1.0f
    );


    // The Projection matrix goes from Camera Space to NDC.
    // So inverse(ProjectionMatrix) goes from NDC to Camera Space.
    glm::mat4 InverseProjectionMatrix = glm::inverse(ProjectionMatrix);

    // The View Matrix goes from World Space to Camera Space.
    // So inverse(ViewMatrix) goes from Camera Space to World Space.
    glm::mat4 InverseViewMatrix = glm::inverse(ViewMatrix);

    glm::vec4 lRayStart_camera = InverseProjectionMatrix * lRayStart_NDC;
    lRayStart_camera/=lRayStart_camera.w;
    glm::vec4 lRayStart_world  = InverseViewMatrix       * lRayStart_camera;
    lRayStart_world /=lRayStart_world .w;
    glm::vec4 lRayEnd_camera   = InverseProjectionMatrix * lRayEnd_NDC;
    lRayEnd_camera  /=lRayEnd_camera  .w;
    glm::vec4 lRayEnd_world    = InverseViewMatrix       * lRayEnd_camera;
    lRayEnd_world   /=lRayEnd_world   .w;

    glm::vec3 lRayDir_world(lRayEnd_world - lRayStart_world);
    lRayDir_world = glm::normalize(lRayDir_world);


    out_origin = glm::vec3(lRayStart_world);
    out_direction = glm::normalize(lRayDir_world);
}

bool TestRayOBBIntersection(
    glm::vec3 ray_origin,        // Ray origin, in world space
    glm::vec3 ray_direction,     // Ray direction (NOT target position!), in world space. Must be normalize()'d.
    glm::vec3 aabb_min,          // Minimum X,Y,Z coords of the mesh when not transformed at all.
    glm::vec3 aabb_max,          // Maximum X,Y,Z coords. Often aabb_min*-1 if your mesh is centered, but it's not always the case.
    glm::mat4 ModelMatrix,       // Transformation applied to the mesh (which will thus be also applied to its bounding box)
    float& intersection_distance // Output : distance between ray_origin and the intersection with the OBB
)
{

    // Intersection method from Real-Time Rendering and Essential Mathematics for Games

    float tMin = 0.0f;
   	float tMax = 100000.0f;

    glm::vec3 OBBposition_worldspace(ModelMatrix[3].x, ModelMatrix[3].y, ModelMatrix[3].z);

    glm::vec3 delta = OBBposition_worldspace - ray_origin;

    // Test intersection with the 2 planes perpendicular to the OBB's X axis
    {
        glm::vec3 xaxis(ModelMatrix[0].x, ModelMatrix[0].y, ModelMatrix[0].z);
        float e = glm::dot(xaxis, delta);
        float f = glm::dot(ray_direction, xaxis);

        if ( fabs(f) > 0.001f )  // Standard case
        {

            float t1 = (e+aabb_min.x)/f; // Intersection with the "left" plane
            float t2 = (e+aabb_max.x)/f; // Intersection with the "right" plane
            // t1 and t2 now contain distances betwen ray origin and ray-plane intersections

            // We want t1 to represent the nearest intersection,
            // so if it's not the case, invert t1 and t2
            if (t1>t2)
            {
                float w=t1;
                t1=t2;
                t2=w; // swap t1 and t2
            }

            // tMax is the nearest "far" intersection (amongst the X,Y and Z planes pairs)
            if ( t2 < tMax )
                tMax = t2;
            // tMin is the farthest "near" intersection (amongst the X,Y and Z planes pairs)
            if ( t1 > tMin )
                tMin = t1;

            // And here's the trick :
            // If "far" is closer than "near", then there is NO intersection.
            // See the images in the tutorials for the visual explanation.
            if (tMax < tMin )
                return false;

        }
        else   // Rare case : the ray is almost parallel to the planes, so they don't have any "intersection"
        {
            if(-e+aabb_min.x > 0.0f || -e+aabb_max.x < 0.0f)
                return false;
        }
    }


    // Test intersection with the 2 planes perpendicular to the OBB's Y axis
    // Exactly the same thing than above.
    {
        glm::vec3 yaxis(ModelMatrix[1].x, ModelMatrix[1].y, ModelMatrix[1].z);
        float e = glm::dot(yaxis, delta);
        float f = glm::dot(ray_direction, yaxis);

        if ( fabs(f) > 0.001f )
        {

            float t1 = (e+aabb_min.y)/f;
            float t2 = (e+aabb_max.y)/f;

            if (t1>t2)
            {
                float w=t1;
                t1=t2;
                t2=w;
            }

            if ( t2 < tMax )
                tMax = t2;
            if ( t1 > tMin )
                tMin = t1;
            if (tMin > tMax)
                return false;

        }
        else
        {
            if(-e+aabb_min.y > 0.0f || -e+aabb_max.y < 0.0f)
                return false;
        }
    }


    // Test intersection with the 2 planes perpendicular to the OBB's Z axis
    // Exactly the same thing than above.
    {
        glm::vec3 zaxis(ModelMatrix[2].x, ModelMatrix[2].y, ModelMatrix[2].z);
        float e = glm::dot(zaxis, delta);
        float f = glm::dot(ray_direction, zaxis);

        if ( fabs(f) > 0.001f )
        {

            float t1 = (e+aabb_min.z)/f;
            float t2 = (e+aabb_max.z)/f;

            if (t1>t2)
            {
                float w=t1;
                t1=t2;
                t2=w;
            }

            if ( t2 < tMax )
                tMax = t2;
            if ( t1 > tMin )
                tMin = t1;
            if (tMin > tMax)
                return false;

        }
        else
        {
            if(-e+aabb_min.z > 0.0f || -e+aabb_max.z < 0.0f)
                return false;
        }
    }

    intersection_distance = tMin;
    return true;

}
void checkNoteClick()
{
    glm::vec3 ray_origin;
    glm::vec3 ray_direction;
    ScreenPosToWorldRay(
		widthScreen/2, heigthScreen/2,//widthScreen/2, heigthScreen /2,
        widthScreen, heigthScreen,
        viewVar,
        projectionVar,
        ray_origin,
        ray_direction
    );

    float intersection_distance; // Output of TestRayOBBIntersection()
	glm::vec4 aabb_min = g_VirtualScene["HPPlane005_Plane"].model_matrix * glm::vec4(g_VirtualScene["HPPlane005_Plane"].bbox_min.x,
		g_VirtualScene["HPPlane005_Plane"].bbox_min.y,
		g_VirtualScene["HPPlane005_Plane"].bbox_min.z,
		1.0f);
    glm::vec4 aabb_max = g_VirtualScene["HPPlane005_Plane"].model_matrix * glm::vec4(g_VirtualScene["HPPlane005_Plane"].bbox_max.x,
		g_VirtualScene["HPPlane005_Plane"].bbox_max.y,
		g_VirtualScene["HPPlane005_Plane"].bbox_max.z,
		1.0f);


    // The ModelMatrix transforms :
    // - the mesh to its desired position and orientation
    // - but also the AABB (defined with aabb_min and aabb_max) into an OBB


    //glm::mat4 ModelMatrix =  Matrix_Translate(+3.05f, -0.3f, -1.225f) * Matrix_Scale(0.2f, 0.2f, 0.2f) * Matrix_Rotate_Y(-PI);

	glm::mat4 ModelMatrix = Matrix_Identity();



    if ( TestRayOBBIntersection(
				ray_origin,
                ray_direction,
                aabb_min,
                aabb_max,
                ModelMatrix,
                intersection_distance)
       )
    {
		if(intersection_distance < 1.5f){
          //distancia para interagir
			cliquei = true;

		}

}
}


// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :
