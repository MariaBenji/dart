#include <stdlib.h>
#include <libguile.h>

#include "guile/newick-type.h"

scm_t_bits newick_tag;

SCM make_newick_smob (const PHYLIP_tree& tree)
{
  SCM smob;
  PHYLIP_tree *newick_smob = new PHYLIP_tree (tree);
  SCM_NEWSMOB (smob, newick_tag, newick_smob);
  return smob;
}

static SCM newick_from_file (SCM s_filename)
{
  SCM smob;

  // read alignment from file
  PHYLIP_tree *tree = new PHYLIP_tree();
  char* filename = scm_to_locale_string (s_filename);
  ifstream in(filename);
  tree->read(in);
  free(filename);

  // Create the smob.
  SCM_NEWSMOB (smob, newick_tag, tree);

  // Return
  return smob;
}

static SCM newick_from_string (SCM s_string)
{
  SCM smob;

  // read alignment from file
  PHYLIP_tree *tree = new PHYLIP_tree();
  char* s = scm_to_locale_string (s_string);
  stringstream ss (s, stringstream::in);
  tree->read(ss);
  free(s);

  // Create the smob.
  SCM_NEWSMOB (smob, newick_tag, tree);

  // Return
  return smob;
}

static SCM newick_to_file (SCM tree_smob, SCM s_filename)
{
  PHYLIP_tree *tree = newick_cast_from_scm (tree_smob);

  // write alignment to file
  char* filename = scm_to_locale_string (s_filename);
  ofstream out(filename);
  tree->write(out,0);
  free(filename);

  // Return
  return SCM_UNSPECIFIED;
}

static SCM newick_node_list (SCM tree_smob, vector<Phylogeny::Node> (PHYLIP_tree::*getNodeMethod)())
{
  PHYLIP_tree& tree = *newick_cast_from_scm (tree_smob);
  SCM tree_node_list = SCM_BOOL_F;

  bool list_empty = true;
  vector<Phylogeny::Node> nodes = (tree.*getNodeMethod)();
  for_const_contents (vector<Phylogeny::Node>, nodes, iter) {
      Phylogeny::Node n = *iter;
      const char* n_name = tree.node_specifier(n).c_str();
      SCM single_element_list = scm_list_1(scm_from_locale_string(n_name));
      if (list_empty) {
	tree_node_list = single_element_list;
	list_empty = false;
      } else
	scm_append_x(scm_list_2(tree_node_list,single_element_list));
  }

  // Return
  return tree_node_list;
}

static SCM newick_leaf_list (SCM tree_smob)
{
  return newick_node_list (tree_smob, &PHYLIP_tree::leaf_vector);
}

static SCM newick_ancestor_list (SCM tree_smob)
{
  return newick_node_list (tree_smob, &PHYLIP_tree::ancestor_vector);
}

static SCM newick_branch_list (SCM tree_smob)
{
  PHYLIP_tree& tree = *newick_cast_from_scm (tree_smob);
  SCM tree_branch_list = SCM_BOOL_F;

  bool list_empty = true;
  for_rooted_branches_pre (tree, b) {
    Phylogeny::Node parent = (*b).first, child = (*b).second;
    double length = (*b).length;
    const char* p_name = tree.node_specifier(parent).c_str();
    const char* c_name = tree.node_specifier(child).c_str();

    SCM branch_tuple = scm_list_3 (scm_from_locale_string(p_name), scm_from_locale_string(c_name), scm_from_double(length));
    SCM single_element_list = scm_list_1(branch_tuple);
      if (list_empty) {
	tree_branch_list = single_element_list;
	list_empty = false;
      } else
	scm_append_x(scm_list_2(tree_branch_list,single_element_list));
  }

  // Return
  return tree_branch_list;
}

static size_t free_newick (SCM tree_smob)
{
  struct PHYLIP_tree *tree = (struct PHYLIP_tree *) SCM_SMOB_DATA (tree_smob);
  delete tree;
  return 0;
}

static int print_newick (SCM tree_smob, SCM port, scm_print_state *pstate)
{
  struct PHYLIP_tree *tree = (struct PHYLIP_tree *) SCM_SMOB_DATA (tree_smob);

  sstring tree_string;
  tree->write(tree_string,0);
  scm_puts (tree_string.c_str(), port);

  /* non-zero means success */
  return 1;
}

PHYLIP_tree* newick_cast_from_scm (SCM tree_smob)
{
  scm_assert_smob_type (newick_tag, tree_smob);
  return (PHYLIP_tree *) SCM_SMOB_DATA (tree_smob);
}

// main guile initialization routine
void init_newick_type (void)
{
  newick_tag = scm_make_smob_type ("newick", sizeof (struct PHYLIP_tree));
  scm_set_smob_free (newick_tag, free_newick);
  scm_set_smob_print (newick_tag, print_newick);

  scm_c_define_gsubr ("newick-from-string", 1, 0, 0, (SCM (*)()) newick_from_string);
  scm_c_define_gsubr ("newick-from-file", 1, 0, 0, (SCM (*)()) newick_from_file);
  scm_c_define_gsubr ("newick-to-file", 2, 0, 0, (SCM (*)()) newick_to_file);
  scm_c_define_gsubr ("newick-ancestor-list", 1, 0, 0, (SCM (*)()) newick_ancestor_list);  // returns list of internal node names (including the root, even if it is a tip node)
  scm_c_define_gsubr ("newick-leaf-list", 1, 0, 0, (SCM (*)()) newick_leaf_list);  // returns list of leaf node names (excluding the root)
  scm_c_define_gsubr ("newick-branch-list", 1, 0, 0, (SCM (*)()) newick_branch_list);  // returns list of (parent,child,length) tuples representing branches, sorted in preorder
}
