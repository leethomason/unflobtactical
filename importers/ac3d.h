typedef struct ACPoint_t 
{
    float x, y, z;
} ACPoint;


typedef struct ACNormal_t
{
    float x, y, z;
} ACNormal;


typedef struct ACVertex_t 
{
    float x, y, z;
    ACNormal normal;
} ACVertex;

typedef struct ACUV_t
{
    float u, v;
} ACUV;


typedef struct ACSurface_t
{
    int *vertref;
    ACUV *uvs;
    int num_vertref;
    ACNormal normal;
    int flags;
    int mat;
} ACSurface;

typedef struct ACObject_t
{
    ACPoint loc;
    char *name;
    char *data;
    char *url;
    ACVertex *vertices;
    int num_vert;

    ACSurface *surfaces;
    int num_surf;
    float texture_repeat_x, texture_repeat_y;
    float texture_offset_x, texture_offset_y;

    int num_kids;
    struct ACObject_t **kids;
    float matrix[9];
    int type;
    int texture;
	char* textureName;
} ACObject;

typedef struct ACCol_t
{
    float r, g, b, a;
} ACCol;

typedef struct Material_t
{
    ACCol rgb; /* diffuse **/
    ACCol ambient;
    ACCol specular;
    ACCol emissive;
    float shininess;
    float transparency;
    char *name;
} ACMaterial;

typedef struct ACImage_t
{
    unsigned short width, height, depth;    
    void *data; 
    int index;
    char *name;
    int amask;
    char *origname; /** do not set - set automatically in texture_read function **/

} ACImage;

#define OBJECT_WORLD 999
#define OBJECT_NORMAL 0
#define OBJECT_GROUP 1
#define OBJECT_LIGHT 2

#define SURFACE_SHADED (1<<4)
#define SURFACE_TWOSIDED (1<<5)

#define SURFACE_TYPE_POLYGON (0)
#define SURFACE_TYPE_CLOSEDLINE (1)
#define SURFACE_TYPE_LINE (2)

#ifdef WIN32
#define Prototype  __declspec( dllexport )
#else
#define Prototype
#endif

#define Private static
#define Boolean int

#ifndef TRUE
#define FALSE (0)
#define TRUE (!FALSE)
#endif

#define STRING(s)  (char *)(strcpy((char *)myalloc(strlen(s)+1), s))
#define streq(a,b)  (!strcmp(a,b))
#define myalloc malloc
#define myfree free


Prototype ACObject *ac_load_ac3d(char *filename);
Prototype ACMaterial *ac_palette_get_material(int index);
Prototype ACImage *ac_get_texture(int ind);
Prototype int ac_load_texture(char *name);
Prototype int ac_load_rgb_image(char *fileName);
Prototype void ac_prepare_render();
Prototype int ac_display_list_render_object(ACObject *ob);
Prototype void ac_object_free(ACObject *ob);
