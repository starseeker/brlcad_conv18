#include "common.h"

#include <map>
#include <set>
#include <queue>
#include <list>
#include <iostream>
#include <fstream>

#include "vmath.h"
#include "raytrace.h"
#include "wdb.h"
#include "plot3.h"
#include "opennurbs.h"

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

void plot_face(ON_3dPoint *pt1, ON_3dPoint *pt2, ON_3dPoint *pt3, int r, int g, int b, FILE *c_plot)
{
    point_t p1, p2, p3;
    VSET(p1, pt1->x, pt1->y, pt1->z);
    VSET(p2, pt2->x, pt2->y, pt2->z);
    VSET(p3, pt3->x, pt3->y, pt3->z);
    pl_color(c_plot, r, g, b);
    pdv_3move(c_plot, p1);
    pdv_3cont(c_plot, p2);
    pdv_3move(c_plot, p1);
    pdv_3cont(c_plot, p3);
    pdv_3move(c_plot, p2);
    pdv_3cont(c_plot, p3);
}

struct Mesh_Info {
    ON_3dPointArray points_p0;
    ON_3dPointArray points_q;
    ON_3dPointArray points_inf;
    std::map<size_t, size_t> iteration_of_insert;
    std::map<size_t, std::vector<size_t> > face_pts;
    std::map<size_t, size_t> index_in_next;
    std::map<size_t, size_t> point_valence;
    std::multimap<size_t, size_t> point_neighbors;
    std::map<std::pair<size_t, size_t>, std::set<size_t> > edges_to_faces;
    size_t iteration_cnt;
};

// Kobbelt sqrt(3)-Subdivision, eqn. 1
void find_q_pts(struct Mesh_Info *mesh) {
    std::map<size_t, std::vector<size_t> >::iterator f_it;
    std::vector<size_t>::iterator l_it;
    for(f_it = mesh->face_pts.begin(); f_it != mesh->face_pts.end(); f_it++) {
	ON_3dPoint p1, p2, p3;
        l_it = (*f_it).second.begin();
	p1 = *mesh->points_p0.At((*l_it));
	p2 = *mesh->points_p0.At((*(l_it+1)));
	p3 = *mesh->points_p0.At((*(l_it+2)));
	ON_3dPoint q((p1.x + p2.x + p3.x)/3, (p1.y + p2.y + p3.y)/3, (p1.z + p2.z + p3.z)/3);
	mesh->points_q.Append(q);
    }
}

// Kobbelt sqrt(3)-Subdivision, eqn. 6
fastf_t alpha_n(size_t n) {
    return (4 - 2 * cos(2*M_PI/n))/9;
}

// From Kobbelt sqrt(3)-Subdivision, eqns 7 and 8, solving for pinf
void point_inf(size_t p, struct Mesh_Info *mesh, ON_3dPointArray *p_a) {
     size_t n = mesh->point_valence[p];
     std::multimap<size_t, size_t>::iterator p_it;
     std::pair<std::multimap<size_t, size_t>::iterator, std::multimap<size_t, size_t>::iterator> range;
     range = mesh->point_neighbors.equal_range(p);
     ON_3dPoint psum = ON_3dPoint(0,0,0);
     for(p_it = range.first; p_it != range.second; p_it++) {
        psum = psum + *mesh->points_p0.At((*p_it).second);
     }
     fastf_t alpha = alpha_n(n);
     //
     //                            3 an                 1
     //                 [pinf = ---------- * psum + -------- * p0]
     //                         3 an n + n          3 an + 1
     //
     ON_3dPoint pinf = (3*alpha/((3*alpha+1)*n))*psum + 1/(3*alpha+1) * *mesh->points_p0.At(p);
     p_a->Append(pinf);
}

// Kobbelt sqrt(3)-Subdivision, eqn. 8
void point_subdiv(size_t m, size_t p, struct Mesh_Info *mesh, ON_3dPointArray *p_a) {
     size_t n = mesh->point_valence[p];
     point_inf(p, mesh, &mesh->points_inf);
     fastf_t gamma = pow((2/3 - alpha_n(n)),m);
     ON_3dPoint subdiv_pt = gamma * *mesh->points_p0.At(p) + (1-gamma) * *mesh->points_inf.At(p);
     p_a->Append(subdiv_pt);
}

// Make an edge using consistent vertex ordering
std::pair<size_t, size_t> mk_edge(size_t pt_A, size_t pt_B)
{
    if (pt_A <= pt_B) {
        return std::make_pair(pt_A, pt_B);
    } else {
        return std::make_pair(pt_B, pt_A);
    }
}

void mesh_add_face(size_t pt1, size_t pt2, size_t pt3, size_t face_cnt, struct Mesh_Info *mesh) {
    mesh->face_pts[face_cnt].push_back(pt1);
    mesh->face_pts[face_cnt].push_back(pt2);
    mesh->face_pts[face_cnt].push_back(pt3);
    mesh->edges_to_faces[mk_edge(pt1, pt2)].insert(face_cnt);
    mesh->edges_to_faces[mk_edge(pt2, pt3)].insert(face_cnt);
    mesh->edges_to_faces[mk_edge(pt3, pt1)].insert(face_cnt);
    mesh->point_neighbors.insert(std::pair<size_t, size_t>(pt1,pt2));
    mesh->point_neighbors.insert(std::pair<size_t, size_t>(pt2,pt3));
    mesh->point_neighbors.insert(std::pair<size_t, size_t>(pt3,pt1));
    mesh->point_valence[pt1] += 1;
    mesh->point_valence[pt2] += 1;
    mesh->point_valence[pt3] += 1;
}

void mesh_info_init(struct rt_bot_internal *bot, struct Mesh_Info *mesh)
{
    for (size_t i = 0; i < bot->num_vertices; ++i) {
	mesh->points_p0.Append(ON_3dPoint(&bot->vertices[i*3]));
	mesh->iteration_of_insert[i] = 0;
    }
    for (size_t i = 0; i < bot->num_faces; ++i) {
        mesh_add_face(bot->faces[i*3+0], bot->faces[i*3+1], bot->faces[i*3+2], i, mesh);
    }
    mesh->iteration_cnt = 0;
}

// Find all edges in mesh
void get_all_edges(struct Mesh_Info *mesh, std::set<std::pair<size_t, size_t> > *edges) {
    std::map<size_t, std::vector<size_t> >::iterator f_it;
    std::vector<size_t>::iterator l_it;
    for(f_it = mesh->face_pts.begin(); f_it != mesh->face_pts.end(); f_it++) {
        l_it = (*f_it).second.begin();
        edges->insert(mk_edge((*l_it), (*(l_it+1))));
        edges->insert(mk_edge((*(l_it+1)), (*(l_it+2))));
        edges->insert(mk_edge((*(l_it+2)), (*l_it)));
    }
}

struct Mesh_Info * iterate(struct rt_bot_internal *bot, struct Mesh_Info *prev_mesh) {
    std::map<size_t, std::vector<size_t> >::iterator f_it;
    std::vector<size_t>::iterator l_it;

    struct Mesh_Info *starting_mesh;
    if (!prev_mesh) {
	starting_mesh = new Mesh_Info;
	mesh_info_init(bot, starting_mesh);
    } else {
	starting_mesh = prev_mesh;
    }
    struct Mesh_Info *mesh = new Mesh_Info;

    mesh->iteration_cnt = starting_mesh->iteration_cnt + 1;

    find_q_pts(starting_mesh);

    // Relax old points here
    for(size_t pcnt = 0; pcnt < (size_t)starting_mesh->points_p0.Count(); pcnt++) {
       point_subdiv(mesh->iteration_cnt, pcnt, starting_mesh, &(mesh->points_p0));
       mesh->iteration_of_insert[pcnt] = starting_mesh->iteration_of_insert[pcnt];
    }
    for(f_it = starting_mesh->face_pts.begin(); f_it != starting_mesh->face_pts.end(); f_it++) {
	mesh->points_p0.Append(starting_mesh->points_q[(*f_it).first]);
        ON_3dPoint added = *mesh->points_p0.At(mesh->points_p0.Count() - 1);
	mesh->iteration_of_insert[mesh->points_p0.Count()-1] = mesh->iteration_cnt;
        starting_mesh->index_in_next[(*f_it).first] = mesh->points_p0.Count()-1;
    }
     // Use the old faces to guide the insertion of the new.
     std::set<std::pair<size_t, size_t> > old_edges;
     std::set<std::pair<size_t, size_t> >::iterator e_it;
     get_all_edges(starting_mesh, &old_edges);
     size_t face_cnt = 0;
     for(f_it = starting_mesh->face_pts.begin(); f_it != starting_mesh->face_pts.end(); f_it++) {
	 std::set<std::pair<size_t, size_t> > face_old_edges;
	 size_t q0 = starting_mesh->index_in_next[(*f_it).first];
	 l_it = (*f_it).second.begin();
	 face_old_edges.insert(std::make_pair((*l_it), (*(l_it+1))));
	 face_old_edges.insert(std::make_pair((*(l_it+1)), (*(l_it+2))));
	 face_old_edges.insert(std::make_pair((*(l_it+2)), (*l_it)));
	 for(e_it = face_old_edges.begin(); e_it != face_old_edges.end(); e_it++) {
	     std::pair<size_t, size_t> edge = mk_edge((*e_it).first, (*e_it).second);
	     if (old_edges.find((edge)) != old_edges.end()) {
		 std::set<size_t> curr_faces = starting_mesh->edges_to_faces[edge];
		 curr_faces.erase((*f_it).first);
		 size_t q1 = starting_mesh->index_in_next[*curr_faces.begin()];
		 mesh_add_face((*e_it).first, q1, q0, face_cnt, mesh);
		 face_cnt++;
		 mesh_add_face((*e_it).second, q0, q1, face_cnt, mesh);
		 face_cnt++;
		 old_edges.erase(edge);
	     }
	 }
     }
     delete starting_mesh;
     return mesh;
}

int main(int argc, char *argv[])
{
    struct db_i *dbip;
    struct directory *dp;
    struct rt_db_internal intern;
    struct rt_bot_internal *bot_ip = NULL;
    struct rt_wdb *wdbp;
    struct bu_vls name;
    struct bu_vls bname;
    struct Mesh_Info *prev_mesh = NULL;
    struct Mesh_Info *mesh = NULL;

    bu_vls_init(&name);

    if (argc != 3) {
	bu_exit(1, "Usage: %s file.g object", argv[0]);
    }

    dbip = db_open(argv[1], "r+w");
    if (dbip == DBI_NULL) {
	bu_exit(1, "ERROR: Unable to read from %s\n", argv[1]);
    }

    if (db_dirbuild(dbip) < 0)
	bu_exit(1, "ERROR: Unable to read from %s\n", argv[1]);

    dp = db_lookup(dbip, argv[2], LOOKUP_QUIET);
    if (dp == RT_DIR_NULL) {
	bu_exit(1, "ERROR: Unable to look up object %s\n", argv[2]);
    }

    RT_DB_INTERNAL_INIT(&intern)
    if (rt_db_get_internal(&intern, dp, dbip, NULL, &rt_uniresource) < 0) {
	bu_exit(1, "ERROR: Unable to get internal representation of %s\n", argv[2]);
    }

    if (intern.idb_minor_type != DB5_MINORTYPE_BRLCAD_BOT) {
	bu_exit(1, "ERROR: object %s does not appear to be of type BoT\n", argv[2]);
    } else {
	bot_ip = (struct rt_bot_internal *)intern.idb_ptr;
    }
    RT_BOT_CK_MAGIC(bot_ip);

    for (size_t i_cnt = 1; i_cnt < 7; i_cnt++) {
        mesh = iterate(bot_ip, prev_mesh);
        prev_mesh = mesh;

        // Plot results
	struct bu_vls fname;
	bu_vls_init(&fname);
	bu_vls_printf(&fname, "root3_%d.pl", i_cnt);
	FILE* plot_file = fopen(bu_vls_addr(&fname), "w");
	std::map<size_t, std::vector<size_t> >::iterator f_it;
	std::vector<size_t>::iterator l_it;
	int r = int(256*drand48() + 1.0);
	int g = int(256*drand48() + 1.0);
	int b = int(256*drand48() + 1.0);
	for(f_it = mesh->face_pts.begin(); f_it != mesh->face_pts.end(); f_it++) {
	    l_it = (*f_it).second.begin();
	    plot_face(&mesh->points_p0[(*l_it)], &mesh->points_p0[(*(l_it+1))], &mesh->points_p0[(*(l_it+2))], r, g ,b, plot_file);
	}
	fclose(plot_file);
    }

    // When constructing the final BoT, use the limit points for all
    // vertices
    ON_3dPointArray points_inf;
    for(size_t v = 0; v < (size_t)mesh->points_p0.Count(); v++) {
	point_inf(v, mesh, &points_inf);
    }
    // The subdivision process shrinks the bot relative to its original
    // vertex positions - to better approximate the original surface,
    // average the change in position of the original vertices to get a
    // scaling factor and apply it to all points in the final mesh.
    fastf_t scale = 0.0;
    for(size_t pcnt = 0; pcnt < bot_ip->num_vertices; pcnt++) {
	ON_3dVector v1(ON_3dPoint(&bot_ip->vertices[pcnt*3]));
	ON_3dVector v2(*points_inf.At(pcnt));
	scale += 1 + (v1.Length() - v2.Length())/v1.Length();
    }
    scale = scale / bot_ip->num_vertices;
    for(size_t pcnt = 0; pcnt < (size_t)points_inf.Count(); pcnt++) {
       ON_3dPoint p0(*points_inf.At(pcnt));
       ON_3dPoint p1 = p0 * scale;
       *points_inf.At(pcnt) = p1;
    }

    wdbp = wdb_dbopen(dbip, RT_WDB_TYPE_DB_DISK);

    fastf_t *vertices = (fastf_t *)bu_malloc(sizeof(fastf_t) * points_inf.Count() * 3, "new verts");
    int *faces = (int *)bu_malloc(sizeof(int) * mesh->face_pts.size() * 3, "new faces");
    for(size_t v = 0; v < (size_t)points_inf.Count(); v++) {
       vertices[v*3] = points_inf[v].x;
       vertices[v*3+1] = points_inf[v].y;
       vertices[v*3+2] = points_inf[v].z;
    }
    std::map<size_t, std::vector<size_t> >::iterator f_it;
    std::vector<size_t>::iterator l_it;
    for(f_it = mesh->face_pts.begin(); f_it != mesh->face_pts.end(); f_it++) {
        l_it = (*f_it).second.begin();
        faces[(*f_it).first*3] = (*l_it);
        faces[(*f_it).first*3+1] = (*(l_it + 1));
        faces[(*f_it).first*3+2] = (*(l_it + 2));
    }

    bu_vls_init(&bname);
    bu_vls_sprintf(&bname, "%s_subd", argv[2]);
    mk_bot(wdbp, bu_vls_addr(&bname), RT_BOT_SOLID, RT_BOT_UNORIENTED, 0, points_inf.Count(), mesh->face_pts.size(), vertices, faces, (fastf_t *)NULL, (struct bu_bitv *)NULL);
    wdb_close(wdbp);
    bu_vls_free(&bname);

    bu_free(vertices, "free subdivision BoT vertices");
    bu_free(faces, "free subdivision BoT faces");

    return 0;
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
