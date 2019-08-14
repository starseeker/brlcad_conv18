// This evolved from the original trimesh halfedge data structure code:

// Author: Yotam Gingold <yotam (strudel) yotamgingold.com>
// License: Public Domain.  (I, Yotam Gingold, the author, release this code into the public domain.)
// GitHub: https://github.com/yig/halfedge
//
// It ended up rather different, as we are concerned with somewhat different
// mesh properties for CDT and the halfedge data structure didn't end up being
// a good fit, but as it's derived from that source the public domain license
// is maintained for the changes as well.

#ifndef __cmesh_h__
#define __cmesh_h__

#include "common.h"

#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include "poly2tri/poly2tri.h"
#include "opennurbs.h"
#include "bu/color.h"

extern "C" {
    struct ctriangle_t {
	long v[3];
	bool isect_edge;
	bool uses_uncontained;
	bool contains_uncontained;
	double angle_to_nearest_uncontained;
    };
}

namespace cmesh
{

struct edge_t {
    long v[2];

    long& start() { return v[0]; }
    const long& start() const {	return v[0]; }
    long& end() { return v[1]; }
    const long& end() const { return v[1]; }

    edge_t(long i, long j)
    {
	v[0] = i;
	v[1] = j;
    }

    edge_t()
    {
	v[0] = v[1] = -1;
    }

    void set(long i, long j)
    {
	v[0] = i;
	v[1] = j;
    }

    bool operator<(edge_t other) const
    {
	bool c1 = (v[0] < other.v[0]);
	bool c1e = (v[0] == other.v[0]);
	bool c2 = (v[1] < other.v[1]);
	return (c1 || (c1e && c2));
    }
    bool operator==(edge_t other) const
    {
	bool c1 = (v[0] == other.v[0]);
	bool c2 = (v[1] == other.v[1]);
	return (c1 && c2);
    }
    bool operator!=(edge_t other) const
    {
	bool c1 = (v[0] != other.v[0]);
	bool c2 = (v[1] != other.v[1]);
	return (c1 || c2);
    }
};

struct uedge_t {
    long v[2];

    uedge_t()
    {
	v[0] = v[1] = -1;
    }

    uedge_t(struct edge_t e)
    {
	v[0] = (e.v[0] <= e.v[1]) ? e.v[0] : e.v[1];
	v[1] = (e.v[0] > e.v[1]) ? e.v[0] : e.v[1];
    }

    uedge_t(long i, long j)
    {
	v[0] = (i <= j) ? i : j;
	v[1] = (i > j) ? i : j;
    }

    void set(long i, long j)
    {
	v[0] = (i <= j) ? i : j;
	v[1] = (i > j) ? i : j;
    }

    bool operator<(uedge_t other) const
    {
	bool c1 = (v[0] < other.v[0]);
	bool c1e = (v[0] == other.v[0]);
	bool c2 = (v[1] < other.v[1]);
	return (c1 || (c1e && c2));
    }
    bool operator==(uedge_t other) const
    {
	bool c1 = ((v[0] == other.v[0]) || (v[0] == other.v[1]));
	bool c2 = ((v[1] == other.v[0]) || (v[1] == other.v[1]));
	return (c1 && c2);
    }
    bool operator!=(uedge_t other) const
    {
	bool c1 = ((v[0] != other.v[0]) && (v[0] != other.v[1]));
	bool c2 = ((v[1] != other.v[0]) && (v[1] != other.v[1]));
	return (c1 || c2);
    }
};

struct triangle_t {
    long v[3];

    long& i() { return v[0]; }
    const long& i() const { return v[0]; }
    long& j() {	return v[1]; }
    const long& j() const { return v[1]; }
    long& k() { return v[2]; }
    const long& k() const { return v[2]; }

    triangle_t(long i, long j, long k)
    {
	v[0] = i;
	v[1] = j;
	v[2] = k;
    }

    triangle_t()
    {
	v[0] = v[1] = v[2] = -1;
    }

    bool operator<(triangle_t other) const
    {
	std::vector<long> vca, voa;
	for (int i = 0; i < 3; i++) {
	    vca.push_back(v[i]);
	    voa.push_back(other.v[i]);
	}
	std::sort(vca.begin(), vca.end());
	std::sort(voa.begin(), voa.end());
	bool c1 = (vca[0] < voa[0]);
	bool c1e = (vca[0] == voa[0]);
	bool c2 = (vca[1] < voa[1]);
	bool c2e = (vca[1] == voa[1]);
	bool c3 = (vca[2] < voa[2]);
	return (c1 || (c1e && c2) || (c1e && c2e && c3));
    }
    bool operator==(triangle_t other) const
    {
	bool c1 = ((v[0] == other.v[0]) || (v[0] == other.v[1]) || (v[0] == other.v[2]));
	bool c2 = ((v[1] == other.v[0]) || (v[1] == other.v[1]) || (v[1] == other.v[2]));
	bool c3 = ((v[2] == other.v[0]) || (v[2] == other.v[1]) || (v[2] == other.v[2]));
	return (c1 && c2 && c3);
    }
    bool operator!=(triangle_t other) const
    {
	bool c1 = ((v[0] != other.v[0]) && (v[0] != other.v[1]) && (v[0] != other.v[2]));
	bool c2 = ((v[1] != other.v[0]) && (v[1] != other.v[1]) && (v[1] != other.v[2]));
	bool c3 = ((v[2] != other.v[0]) && (v[2] != other.v[1]) && (v[2] != other.v[2]));
	return (c1 || c2 || c3);
    }

};

class cmesh_t
{
public:

    /* Data containers */
    std::vector<ON_3dPoint *> pnts;
    std::map<ON_3dPoint *, long> p2ind;
    std::set<triangle_t> tris;
    std::map<long, std::set<edge_t>> v2edges;
    std::map<long, std::set<triangle_t>> v2tris;
    std::map<edge_t, triangle_t> edges2tris;
    std::map<uedge_t, std::set<triangle_t>> uedges2tris;

    /* Setup */
    void build(std::set<p2t::Triangle *> *cdttri, std::map<p2t::Point *, ON_3dPoint *> *pointmap);
    void set_brep_data(
	    bool brev,
	    std::set<ON_3dPoint *> *e,
	    std::set<ON_3dPoint *> *s,
	    std::map<ON_3dPoint *, ON_3dPoint *> *n
	    );

    /* Mesh data sets */
    std::set<uedge_t> boundary_edges(int use_brep_data);
    std::vector<triangle_t> face_neighbors(const triangle_t &f);
    std::vector<triangle_t> vertex_face_neighbors(long vind);

    std::vector<triangle_t> singularity_triangles();

    std::vector<triangle_t> interior_incorrect_normals(int use_brep_data);
    std::vector<triangle_t> problem_edge_tris();
    bool self_intersecting_mesh();

    std::set<triangle_t> seed_tris;

    // Plot3 generation routines for debugging
    void boundary_edges_plot(const char *filename);
    void face_neighbors_plot(const triangle_t &f, const char *filename);
    void vertex_face_neighbors_plot(long vind, const char *filename);
    void interior_incorrect_normals_plot(const char *filename);
    void tris_set_plot(std::set<triangle_t> &tset, const char *filename);
    void tris_vect_plot(std::vector<triangle_t> &tvect, const char *filename);
    void ctris_vect_plot(std::vector<struct ctriangle_t> &tvect, const char *filename);
    void tris_plot(const char *filename);
    void tri_plot(triangle_t &tri, const char *filename);

    // Triangle geometry information
    ON_3dPoint tcenter(const triangle_t &t);
    ON_3dVector bnorm(const triangle_t &t);
    ON_3dVector tnorm(const triangle_t &t);
    std::set<uedge_t> uedges(const triangle_t &t);

    bool brep_edge_pnt(long v);

    // Trigger mesh repair logic
    void repair();

    int op_cnt;

private:
    // For situations where we need to process using Brep data
    std::set<ON_3dPoint *> *edge_pnts;
    std::set<ON_3dPoint *> *singularities;
    std::set<long> sv; // Singularity vertex indices
    std::map<ON_3dPoint *, ON_3dPoint *> *normalmap;
    bool m_bRev;

    // Boundary edge information
    std::set<uedge_t> current_bedges;
    std::set<uedge_t> problem_edges;
    std::map<long, std::set<edge_t>> edge_pnt_edges;
    edge_t find_boundary_oriented_edge(uedge_t &ue);

    // Submesh building
    bool tri_problem_edges(triangle_t &t);
    double max_angle_delta(triangle_t &seed, std::vector<triangle_t> &s_tris);
    void process_seed_tri(triangle_t &seed, bool repair, double deg);

    // Mesh manipulation functions
    bool tri_add(triangle_t &atris);
    void tri_remove(triangle_t &etris);
    void reset();

    // Plotting utility functions
    void plot_tri(const triangle_t &t, struct bu_color *buc, FILE *plot, int r, int g, int b);
    void plot_uedge(struct uedge_t &ue, FILE* plot_file);
};

class cpolyedge_t
{
    public:
	cpolyedge_t *prev;
	cpolyedge_t *next;

	long v[2];

	cpolyedge_t(edge_t &e){
	    v[0] = e.v[0];
	    v[1] = e.v[1];
	    prev = NULL;
	    next = NULL;
	};
};

class cpolygon_t
{
    public:
	bool closed();
	bool self_intersecting();
	bool cdt();

	bool point_in_polygon(long v, bool flip);
	// Apply the point-in-polygon test to all uncontained points, moving any inside the loop into interior_points
	bool have_uncontained();

	long tri_process(std::set<edge_t> *ne, std::set<edge_t> *se, long *nv, triangle_t &t);
	long add_edge(const struct edge_t &e);
	void remove_edge(const struct edge_t &e);
	long replace_edges(std::set<edge_t> &new_edges, std::set<edge_t> &old_edges);

	void polygon_plot(const char *filename);
	void polygon_plot_in_plane(const char *filename);
	void print();

	std::set<triangle_t> tris;
	std::set<cpolyedge_t *> poly;
	std::map<long, std::set<cpolyedge_t *>> v2pe;
	std::set<long> interior_points;
	std::set<long> uncontained;
	std::set<long> flipped_face;
	point2d_t *pnts_2d;

	std::set<uedge_t> active_edges;
	std::set<uedge_t> problem_edges;
	std::set<uedge_t> self_isect_edges;

	ON_Plane tplane;

	cmesh_t *cmesh;

	long shared_edge_cnt(triangle_t &t);
	long unshared_vertex(triangle_t &t);
	std::pair<long,long> shared_vertices(triangle_t &t);
	double ucv_angle(triangle_t &t);

};

class csweep_t
{
    public:
	cpolygon_t polygon;
	std::set<uedge_t> interior_uedges;

	// Project cmesh 3D points into a 2D point array.  Probably won't use all of
	// them, but this way vert indices on triangles will match in 2D and 3D.
	void build_2d_pnts(ON_3dPoint &c, ON_3dVector &n);

	// An initial loop may not contain all the interior points - to ensure it does,
	// use grow_loop with a large deg value and the stop_on_contained flag set.
	bool build_initial_loop(triangle_t &seed, bool repair);

	// To grow only until all interior points are within the polygon, supply true
	// for stop_on_contained.  Otherwise, grow_loop will follow the triangles out
	// until the Brep normals of the triangles are beyond the deg limit.  Note
	// that triangles which would cause a self-intersecting polygon will be
	// rejected, even if they satisfy deg.
	long grow_loop(double deg, bool stop_on_contained);

	cmesh_t *cmesh;

	void plot_uedge(struct uedge_t &ue, FILE* plot_file);
	void plot_tri(const triangle_t &t, struct bu_color *buc, FILE *plot, int r, int g, int b);
	void face_neighbors_plot(const triangle_t &f, const char *filename);
	void vertex_face_neighbors_plot(long vind, const char *filename);

	std::set<triangle_t> visited_triangles;
	std::set<triangle_t> unusable_triangles;

    private:
	ON_Plane tplane;
	ON_3dVector pdir;

	std::vector<struct ctriangle_t> polygon_tris(double angle, bool brep_norm);

};



}

#endif /* __trimesh_h__ */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
