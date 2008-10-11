#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#include "ac3d.h"


static int line = 0;
static char buff[255];

static ACMaterial palette[255];
static int num_palette = 0;
static int startmatindex = 0;


ACMaterial *ac_palette_get_material(int id)
{
    return(&palette[id]);
}


Boolean read_line(FILE *f)
{
    fgets(buff, 255, f); line++;
    return(TRUE);
}


int tokc = 0;
char *tokv[30];

Prototype int get_tokens(char *s, int *argc, char *argv[])
/** bung '\0' chars at the end of tokens and set up the array (tokv) and count (tokc)
	like argv argc **/
{
    char *p = s;
    char *st;
    char c;
    //int n;
    int tc;

    tc = 0;
    while ((c=*p) != 0)
    {
	if ((c != ' ') && (c != '\t') && (c != '\n') && ( c != 13))
	{
	    if (c == '"')
	    {
		c = *p++;
		st = p;
		while ((c = *p) && ((c != '"')&&(c != '\n')&& ( c != 13)) )
		{
		    if (c == '\\')
			strcpy(p, p+1);
		    p++;
		}
		*p=0;
		argv[tc++] = st;
	    }
	    else
	    {
		st = p;
		while ((c = *p) && ((c != ' ') && (c != '\t') && (c != '\n') && ( c != 13)) )
		    p++;
		*p=0;
		argv[tc++] = st;
	    }			
	}
	p++;
    }

    *argc = tc;
    return(tc);
}


ACObject *new_object()
{
    ACObject *ob = (ACObject *)myalloc(sizeof(ACObject));

    ob->loc.x = ob->loc.y = ob->loc.z = 0.0;
    ob->name = ob->url = NULL;
    ob->data = NULL;
    ob->vertices = NULL;
    ob->num_vert = 0;
    ob->surfaces = NULL;
    ob->num_surf = 0;
    ob->texture = -1;
    ob->texture_repeat_x = ob->texture_repeat_y = 1.0;
    ob->texture_offset_x = ob->texture_offset_y = 0.0;
    ob->kids = NULL;
    ob->num_kids = 0;
    ob->matrix[0] = 1;
    ob->matrix[1] = 0;
    ob->matrix[2] = 0;
    ob->matrix[3] = 0;
    ob->matrix[4] = 1;
    ob->matrix[5] = 0;
    ob->matrix[6] = 0;
    ob->matrix[7] = 0;
    ob->matrix[8] = 1;
    return(ob);
}


void ac_object_free(ACObject *ob)
{
int n, s;

	for (n = 0; n < ob->num_kids; n++)
		ac_object_free(ob->kids[n]);

	if (ob->vertices)
		free(ob->vertices);

	for (s = 0; s < ob->num_surf; s ++)
		{
		free(ob->surfaces[s].vertref);
		free(ob->surfaces[s].uvs);
		}

	if (ob->surfaces)
		free(ob->surfaces);

	if (ob->data)
		free(ob->data);

	if (ob->url)
		free(ob->url);

	if (ob->name)
		free(ob->name);

	if (ob->textureName)
		free(ob->textureName);

	free(ob);

}

void init_surface(ACSurface *s)
{
    s->vertref = NULL;
    s->uvs = NULL;
    s->num_vertref = 0;
    s->flags = 0;
    s->mat = 0;
    s->normal.x = 0.0; s->normal.z = 0.0; s->normal.z = 0.0; 
}



void tri_calc_normal(ACPoint *v1, ACPoint *v2, ACPoint *v3, ACPoint *n)
{
    double len;

    n->x = (v2->y-v1->y)*(v3->z-v1->z)-(v3->y-v1->y)*(v2->z-v1->z);
    n->y = (v2->z-v1->z)*(v3->x-v1->x)-(v3->z-v1->z)*(v2->x-v1->x);
    n->z = (v2->x-v1->x)*(v3->y-v1->y)-(v3->x-v1->x)*(v2->y-v1->y);
    len = sqrt(n->x*n->x + n->y*n->y + n->z*n->z);

    if (len > 0)
    {
	n->x /= (float)len;
	n->y /= (float)len;
	n->z /= (float)len;  
    }

}



ACSurface *read_surface(FILE *f, ACSurface *s, ACObject *ob)
{
    char t[20];

    init_surface(s);

    while (!feof(f))
    {
	read_line(f);
	sscanf(buff, "%s", t);

	if (streq(t, "SURF"))
        {
	    int flgs;

	    if (get_tokens(buff, &tokc, tokv) != 2)
            {
		printf("SURF should be followed by one flags argument\n");
            }
	    else
            {
		flgs = strtol(tokv[1], NULL, 0);
		s->flags = flgs;
            }
        }
	else
	    if (streq(t, "mat"))
	    {
		int mindx;

		sscanf(buff, "%s %d", t, &mindx);
		s->mat = mindx+startmatindex;
	    }
	    else
		if (streq(t, "refs"))
		{
		    int num, n;
		    int ind;
		    float tx, ty;
  
		    sscanf(buff, "%s %d", t, &num);        

		    s->num_vertref = num;
		    s->vertref = (int *)malloc( num * sizeof(int));
		    s->uvs = (ACUV *)malloc( num * sizeof(ACUV));

		    for (n = 0; n < num; n++)
		    {
			fscanf(f, "%d %f %f\n", &ind, &tx, &ty); line++;
			s->vertref[n] = ind;
			s->uvs[n].u = tx;
			s->uvs[n].v = ty;
		    }

		    /** calc surface normal **/
		    if (s->num_vertref >= 3)
			tri_calc_normal((ACPoint *)&ob->vertices[s->vertref[0]], 
					(ACPoint *)&ob->vertices[s->vertref[1]], 
					(ACPoint *)&ob->vertices[s->vertref[2]], (ACPoint *)&s->normal);

		    return(s);
		}
		else
		    printf("ignoring %s\n", t);

    }
    return(NULL);
}


void ac_object_calc_vertex_normals(ACObject *ob)
{
    int s, v, vr;

    /** for each vertex in this object **/
    for (v = 0; v < ob->num_vert; v++)
    {
	ACNormal n = {0, 0, 0};
	int found = 0;

	/** go through each surface **/
	for (s = 0; s < ob->num_surf; s++)
	{
	    ACSurface *surf = &ob->surfaces[s];

	    /** check if this vertex is used in this surface **/
	    /** if it is, use it to create an average normal **/
	    for (vr = 0; vr < surf->num_vertref; vr++)
		if (surf->vertref[vr] == v)
		{
		    n.x+=surf->normal.x;
		    n.y+=surf->normal.y;
		    n.z+=surf->normal.z;
		    found++;
		}
	}
	if (found > 0)
	{
	    n.x /= found;
	    n.y /= found;
	    n.z /= found;
	}
	ob->vertices[v].normal = n;
    }
	

}


int string_to_objecttype(char *s)
{
    if (streq("world", s))
        return(OBJECT_WORLD);
    if (streq("poly", s))
        return(OBJECT_NORMAL);
    if (streq("group", s))
        return(OBJECT_GROUP);
    if (streq("light", s))
        return(OBJECT_LIGHT);
    return(OBJECT_NORMAL);
}

ACObject *ac_load_object(FILE *f, ACObject *parent)
{
    char t[20];
    ACObject *ob = NULL;

    while (!feof(f))
    {
	read_line(f);

	sscanf(buff, "%s", t);

        if (streq(t, "MATERIAL"))
	{
            float shi, tran;
            ACMaterial m;
	    
            if (get_tokens(buff, &tokc, tokv) != 22)
	    {
                printf("expected 21 params after \"MATERIAL\" - line %d\n", line);
	    }
            else
	    {
		m.name = STRING(tokv[1]);
		m.rgb.r = (float)atof(tokv[3]);
		m.rgb.g = (float)atof(tokv[4]);
		m.rgb.b = (float)atof(tokv[5]);

		m.ambient.r = (float)atof(tokv[7]);
		m.ambient.g = (float)atof(tokv[8]);
		m.ambient.b = (float)atof(tokv[9]);

		m.emissive.r = (float)atof(tokv[11]);
		m.emissive.g = (float)atof(tokv[12]);
		m.emissive.b = (float)atof(tokv[13]);

		m.specular.r = (float)atof(tokv[15]);
		m.specular.g = (float)atof(tokv[16]);
		m.specular.b = (float)atof(tokv[17]);

		m.shininess = (float)atof(tokv[19]);
		m.transparency = (float)atof(tokv[21]);

		shi = (float)atof(tokv[6]);
		tran = (float)atof(tokv[7]);

		palette[num_palette++] = m;

	    }
	}
        else
	    if (streq(t, "OBJECT"))
            {
		char type[20];
		char str[20];
		ob = new_object();

		sscanf(buff, "%s %s", str, type);
		
		ob->type = string_to_objecttype(type);
            }
	    else
		if (streq(t, "data"))
		{
		    if (get_tokens(buff, &tokc, tokv) != 2)
			printf("expected 'data <number>' at line %d\n", line);
		    else
		    {
			char *str;
			int len;

			len = atoi(tokv[1]);
			if (len > 0)
			{
			    str = (char *)myalloc(len+1);
			    fread(str, len, 1, f);
			    str[len] = 0;
			    fscanf(f, "\n"); line++;
			    ob->data = STRING(str);
			    myfree(str);
			}
		    }
		}
		else
		    if (streq(t, "name"))
		    {
			int numtok = get_tokens(buff, &tokc, tokv);
			if (numtok != 2)
			{
			    printf("expected quoted name at line %d (got %d tokens)\n", line, numtok);
			}
			else
			    ob->name = STRING(tokv[1]);
		    }
		    else
			if (streq(t, "texture"))
			{
			    if (get_tokens(buff, &tokc, tokv) != 2)
				printf("expected quoted texture name at line %d\n", line);

			    else
			    {
				//ob->texture = ac_load_texture(tokv[1]);
				ob->textureName = STRING(tokv[1]);
			    }
			}
			else
			    if (streq(t, "texrep"))
			    {
				if (get_tokens(buff, &tokc, tokv) != 3)
				    printf("expected 'texrep <float> <float>' at line %d\n", line);
				else
				{
				    ob->texture_repeat_x = (float)atof(tokv[1]);
				    ob->texture_repeat_y = (float)atof(tokv[2]);
				}
			    }
			    else
				if (streq(t, "texoff"))
				{
				    if (get_tokens(buff, &tokc, tokv) != 3)
					printf("expected 'texoff <float> <float>' at line %d\n", line);
				    else
				    {
					ob->texture_offset_x = (float)atof(tokv[1]);
					ob->texture_offset_y = (float)atof(tokv[2]);
				    }
				}
				else
				    if (streq(t, "rot"))
				    {
					float r[9];
					char str2[5];
					int n;

					sscanf(buff, "%s %f %f %f %f %f %f %f %f %f", str2, 
					       &r[0], &r[1], &r[2], &r[3], &r[4], &r[5], &r[6], &r[7], &r[8] );

					for (n = 0; n < 9; n++)
					    ob->matrix[n] = r[n];

				    }
				    else
					if (streq(t, "loc"))
					{
					    char str[5];
					    sscanf(buff, "%s %f %f %f", str,
						   &ob->loc.x, &ob->loc.y, &ob->loc.z);			
					}
					else
					    if (streq(t, "url"))
					    {
						if (get_tokens(buff, &tokc, tokv) != 2)
						    printf("expected one arg to url at line %d (got %s)\n", line, tokv[0]);
						else
						    ob->url = STRING(tokv[1]);
					    }
					    else
						if (streq(t, "numvert"))
						{
						    int num, n;
						    char str[10];

						    sscanf(buff, "%s %d", str, &num);

						    if (num > 0)
						    {
							ob->num_vert = num;
							ob->vertices = (ACVertex *)myalloc(sizeof(ACVertex)*num);

							for (n = 0; n < num; n++)
							{
							    ACVertex p;
							    fscanf(f, "%f %f %f\n", &p.x, &p.y, &p.z); line++;
							    ob->vertices[n] = p;
							}

						    }
						}
						else
						    if (streq(t, "numsurf"))
						    {
							int num, n;
							char str[10];

							sscanf(buff, "%s %d", str, &num);
							if (num > 0)
							{
							    ob->num_surf = num;
							    ob->surfaces = (ACSurface *)myalloc(sizeof(ACSurface) * num);

							    for (n = 0; n < num; n++)
							    {
								ACSurface *news = read_surface(f, &ob->surfaces[n], ob);
								if (news == NULL)
								{
								    printf("error whilst reading surface at line: %d\n", line);
								    return(NULL);
								}
						
							    }
							}
						    }
						    else
							if (streq(t, "kids")) /** 'kids' is the last token in an object **/
							{
							    int num, n;

							    sscanf(buff, "%s %d", t, &num);
			
							    if (num != 0)
							    {
								ob->kids = (ACObject **)myalloc(num * sizeof(ACObject *) );
								ob->num_kids = num;

								for (n = 0; n < num; n++)
								{
								    ACObject *k = ac_load_object(f, ob);

								    if (k == NULL)
								    {
									printf("error reading expected child object %d of %d at line: %d\n", n+1, num, line);
									return(ob);
								    }
								    else
									ob->kids[n] = k;
								}

							    }
							    return(ob);
							}

    }
    return(ob);

}


void ac_calc_vertex_normals(ACObject *ob)
{
    int n;
    
    ac_object_calc_vertex_normals(ob);
    if (ob->num_kids)
	for (n = 0; n < ob->num_kids; n++)
	    ac_calc_vertex_normals(ob->kids[n]);
}


ACObject *ac_load_ac3d(char *fname)
{
    FILE *f = fopen(fname, "r");
    ACObject *ret = NULL;

    if (f == NULL)
    {
	printf("can't open %s\n", fname);
	return(NULL);
    }

    read_line(f);

    if (strncmp(buff, "AC3D", 4))
    {
	printf("ac_load_ac '%s' is not a valid AC3D file.", fname);
	fclose(f);
	return(0);
    }


    startmatindex = num_palette;


    ret = ac_load_object(f, NULL);

	
    fclose(f);

    ac_calc_vertex_normals(ret);


    return(ret);
}







void ac_dump(ACObject *ob)
{
    int n;

    printf("OBJECT name %s\nloc %f %f %f\nnum_vert %d\nnum_surf %d\n",
	   ob->name, ob->loc.x, ob->loc.y, ob->loc.z, ob->num_vert, ob->num_surf);


    for (n=0; n < ob->num_vert; n++)
	printf("\tv %f %f %f\n", ob->vertices[n].x, ob->vertices[n].y, ob->vertices[n].z);

    for (n=0; n < ob->num_surf; n++)
    {
	ACSurface *s = &ob->surfaces[n];
	printf("surface %d, %d refs, mat %d\n", n, s->num_vertref, s->mat);
    }

    if (ob->num_kids)
	for (n = 0; n < ob->num_kids; n++)
	    ac_dump(ob->kids[n]); 
			
}
