#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da cor de cada vértice, definidas em "shader_vertex.glsl" e
// "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento

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
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;
uniform sampler2D TextureImage6;
uniform sampler2D TextureImage7;
uniform sampler2D TextureImage8;
uniform sampler2D TextureImage9;
// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec3 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    vec4 bbox_center;

    float minx = bbox_min.x;
    float maxx = bbox_max.x;

    float miny = bbox_min.y;
    float maxy = bbox_max.y;

    float minz = bbox_min.z;
    float maxz = bbox_max.z;

    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    vec4 r = -1 * l + 2 * n * dot(n,l);

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    vec3 Kd;
    vec3 Ks;
    vec3 Ka;
    float q = 0.0f;

    if ( object_id == WALL)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage0, vec2(U,V)).rgb;
        Ks = vec3(0.0f,0.0f,0.0f);
        Ka = Kd / 2; //vec3(0.0f,0.0f,0.0f);
        q = 1.0f;

    }
    else if (object_id == FLOOR) {
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage1, vec2(U,V)).rgb;
        Ks = vec3(0.0f,0.0f,0.0f);
        Ka = Kd/2;   //vec3(0.0f,0.0f,0.0f);
        q = 1;
    }
    else if (object_id == CEILING) {
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage2, vec2(U,V)).rgb;
        Ks = vec3(0.0f,0.0f,0.0f);
        Ka = Kd/2; //vec3(0.0f,0.0f,0.0f);
        q = 1;
    }
    else if (object_id == COW){
        U = (position_model.x - minx)/(maxx - minx) ;
        V = (position_model.y - miny)/(maxy - miny) ;

        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        Kd = texture(TextureImage3, vec2(U,V)).rgb;
        Ka = vec3(0.1f,0.1f,0.1f);
        Ks = vec3(0.1f,0.1f,0.1f);
        q = 1;
    }
    else if (object_id == SPHERE){
        Kd = vec3(0.08,0.8,0.4);
        Ks = vec3(0.8,0.8,0.8);
        Ka = Kd/2;
        q = 32.0;
    }
      else if (object_id == TABLE){
           U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage8, vec2(U,V)).rgb;
        Ks = vec3(0.0f,0.0f,0.0f);
        Ka = Kd/2;
        q = 1;
    }
      else if (object_id == LAPTOP){
          U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage4, vec2(U,V)).rgb;
        Ks = vec3(0.0f,0.0f,0.0f);
        Ka = Kd/2;
        q = 1;
    }
    else if(object_id == BOMB)
    {
        U = texcoords.x;
        V = texcoords.y;

        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        Kd = texture(TextureImage5, vec2(U,V)).rgb;

        vec3 n_aux = texture(TextureImage6, vec2(U,V)).rbg;
        n = vec4(n_aux.x, n_aux.y, n_aux.z, 0.0f);

        Ks = texture(TextureImage7, vec2(U,V)).rgb;
        float q = 10.0;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float phong = pow(max(0.0,dot(r,v)), q);
        color = (Kd * (lambert + 0.01)) + (Ks * (phong + 0.01));

    }
    else if (object_id == BUTTON){
             U = (position_model.x - minx)/(maxx - minx) ;
        V = (position_model.y - miny)/(maxy - miny) ;


        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        Kd = texture(TextureImage8, vec2(U,V)).rgb;
        Ks = vec3(0.2f,0.2f,0.2f);
        Ka = Kd/2;
        q = 1;
    }
    else if (object_id == DOOR){
            U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage9, vec2(U,V)).rgb;
        Ks = vec3(0.0f,0.0f,0.0f);
        Ka = Kd/2;
        q = 1;
    }



    vec3 I = vec3(1.0,1.0,1.0);

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.2,0.2,0.2);

    // Termo difuso utilizando a lei dos cossenos de Lambert
    vec3 lambert_diffuse_term = Kd * I * max(0, dot(n,l));

    // Termo ambiente
    vec3 ambient_term = Ka * Ia;

    // Termo especular utilizando o modelo de iluminação de Phong
    vec3 phong_specular_term  = Ks * I * pow(max(0.0,dot(r,v)), q);

    color = lambert_diffuse_term + ambient_term + phong_specular_term;



    // Equação de Iluminação
   // float lambert = max(0,dot(n,l));

    //color = Kd0 * (lambert + 0.01);

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
   color = pow(color, vec3(1.0f,1.0f,1.0f)/2.2);
}
