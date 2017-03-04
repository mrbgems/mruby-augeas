#include <memory.h>
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/range.h>
#include <mruby/value.h>
#include <mruby/hash.h>

#include <augeas.h>

static void augeas_free(mrb_state *, void *);

static struct mrb_data_type augeas_type = {
    "Augeas", augeas_free
};

static augeas *
aug_handle(mrb_state *mrb, mrb_value s) {
    augeas *aug = (augeas *) mrb_data_get_ptr(mrb, s, &augeas_type);

    if (aug == NULL) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Failed to retrieve connection");
    }
    return aug;
}

static void
augeas_free(mrb_state *mrb, void *aug) {
    if (aug != NULL)
        aug_close((augeas *) aug);
}

/*
 * call-seq:
 *   get(PATH) -> String
 *
 * Lookup the value associated with PATH
 */
mrb_value
augeas_get(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *cpath = NULL;
    const char *value = NULL;
    int r;

    mrb_get_args(mrb, "z", &cpath);

    r = aug_get(aug, cpath, &value);
    /* There used to be a bug in Augeas that would make it not properly set
     * VALUE to NULL when PATH was invalid. We check RETVAL, too, to avoid
     * running into that */
    if (r == 1 && value != NULL) {
        return mrb_str_new_cstr(mrb, value);
    } else {
        return mrb_nil_value();
    }
}

/*
 * call-seq:
 *   exists(PATH) -> boolean
 *
 * Return true if there is an entry for this path, false otherwise
 */
mrb_value
augeas_exists(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *cpath = NULL;

    mrb_get_args(mrb, "z", &cpath);

    return mrb_bool_value(aug_get(aug, cpath, NULL) == 1);
}

static int
set(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);

    char *path, *value;
    mrb_get_args(mrb, "zz!", &path, &value);

    return aug_set(aug, path, value);
}

/*
 * call-seq:
 *   set(PATH, VALUE) -> int
 *
 * Set the value associated with PATH to VALUE. VALUE is copied into the
 * internal data structure. Intermediate entries are created if they don't
 * exist.
 */
mrb_value
augeas_set(mrb_state *mrb, mrb_value self) {
    return mrb_bool_value(set(mrb, self) == 0);
}

mrb_value
facade_set(mrb_state *mrb, mrb_value self) {
    return mrb_fixnum_value(set(mrb, self));
}

/*
 * call-seq:
 *   setm(BASE, SUB, VALUE) -> boolean
 *
 *  Set multiple nodes in one operation. Find or create a node matching
 *  SUB by interpreting SUB as a path expression relative to each node
 *  matching BASE. SUB may be NULL, in which case all the nodes matching
 *  BASE will be modified.
 */
mrb_value
augeas_setm(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *cbase, *csub, *cvalue;

    mrb_get_args(mrb, "zz!z!", &cbase, &csub, &cvalue);
    return mrb_fixnum_value(aug_setm(aug, cbase, csub, cvalue));
}

/*
 * call-seq:
 *   insert(PATH, LABEL, BEFORE) -> int
 *
 * Make LABEL a sibling of PATH by inserting it directly before or after PATH.
 * The boolean BEFORE determines if LABEL is inserted before or after PATH.
 */
mrb_value
augeas_insert(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *cpath, *clabel;
    mrb_bool before;

    mrb_get_args(mrb, "zzb", &cpath, &clabel, &before);
    return mrb_fixnum_value(aug_insert(aug, cpath, clabel, before));
}

/*
 * call-seq:
 *   mv(SRC, DST) -> int
 *
 * Move the node SRC to DST. SRC must match exactly one node in the
 * tree. DST must either match exactly one node in the tree, or may not
 * exist yet. If DST exists already, it and all its descendants are
 * deleted. If DST does not exist yet, it and all its missing ancestors are
 * created.
 */
mrb_value
augeas_mv(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *csrc, *cdst;

    mrb_get_args(mrb, "zz", &csrc, &cdst);

    return mrb_fixnum_value(aug_mv(aug, csrc, cdst));
}

/*
 * call-seq:
 *   rm(PATH) -> int
 *
 * Remove path and all its children. Returns the number of entries removed
 */
mrb_value
augeas_rm(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *cpath;
    mrb_get_args(mrb, "z", &cpath);

    return mrb_fixnum_value(aug_rm(aug, cpath));
}

/*
 * call-seq:
 *       match(PATH) -> an_array
 *
 * Return all the paths that match the path expression PATH as an aray of
 * strings.
 */
mrb_value
augeas_match(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *path;
    mrb_value result;
    char **matches = NULL;
    int cnt, i;

    mrb_get_args(mrb, "z", &path);

    cnt = aug_match(aug, path, &matches) ;
    if (cnt < 0)
        mrb_raisef(mrb, E_ARGUMENT_ERROR, "Matching path expression '%s' failed",
                   path);

    result = mrb_ary_new(mrb);
    for (i = 0; i < cnt; i++) {
        mrb_ary_push(mrb, result, mrb_str_new_cstr(mrb, matches[i]));
        free(matches[i]);
    }
    free (matches);

    return result;
}

/*
 * call-seq:
 *       match(PATH) -> an_array
 *
 * Return all the paths that match the path expression PATH as an aray of
 * strings.
 * Returns an empty array if no paths were found.
 */
mrb_value
facade_match(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *path;
    char **matches = NULL;
    int cnt, i;
    mrb_value result;

    mrb_get_args(mrb, "z", &path);

    cnt = aug_match(aug, path, &matches) ;
    if (cnt < 0)
        return mrb_fixnum_value(-1);

    result = mrb_ary_new(mrb);
    for (i = 0; i < cnt; i++) {
        mrb_ary_push(mrb, result, mrb_str_new_cstr(mrb, matches[i]));
        free(matches[i]);
    }
    free (matches);

    return result;
}

/*
 * call-seq:
 *       save() -> boolean
 *
 * Write all pending changes to disk
 */
mrb_value
augeas_save(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);

    return mrb_bool_value(aug_save(aug) == 0);
}

/*
 * call-seq:
 *       save() -> int
 *
 * Write all pending changes to disk
 */
mrb_value
facade_save(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    return mrb_fixnum_value(aug_save(aug));
}

/*
 * call-seq:
 *       load() -> boolean
 *
 * Load files from disk according to the transforms under +/augeas/load+
 */
mrb_value
augeas_load(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);

    return mrb_bool_value(aug_load(aug) == 0);
}

/*
 * call-seq:
 *   defvar(NAME, EXPR) -> boolean
 *
 * Define a variable NAME whose value is the result of evaluating EXPR. If
 * a variable NAME already exists, its name will be replaced with the
 * result of evaluating EXPR.
 *
 * If EXPR is NULL, the variable NAME will be removed if it is defined.
 *
 */
mrb_value
augeas_defvar(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *cname, *cexpr;

    mrb_get_args(mrb, "zz!", &cname, &cexpr);

    return mrb_bool_value(aug_defvar(aug, cname, cexpr) >= 0);
}

/*
 * call-seq:
 *   defnode(NAME, EXPR, VALUE) -> boolean
 *
 * Define a variable NAME whose value is the result of evaluating EXPR,
 * which must be non-NULL and evaluate to a nodeset. If a variable NAME
 * already exists, its name will be replaced with the result of evaluating
 * EXPR.
 *
 * If EXPR evaluates to an empty nodeset, a node is created, equivalent to
 * calling AUG_SET(AUG, EXPR, VALUE) and NAME will be the nodeset containing
 * that single node.
 *
 * Returns +false+ if +aug_defnode+ fails, and the number of nodes in the
 * nodeset on success.
 */
mrb_value
augeas_defnode(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *cname, *cexpr, *cvalue;
    int r;

    mrb_get_args(mrb, "zz!z!", &cname, &cexpr, &cvalue);

    /* FIXME: Figure out a way to return created, maybe accept a block
       that gets run when created == 1 ? */
    r = aug_defnode(aug, cname, cexpr, cvalue, NULL);

    return (r < 0) ? mrb_false_value() : mrb_fixnum_value(r);
}

mrb_value
augeas_init(mrb_state *mrb, mrb_value class) {
    unsigned int flags;
    char *root, *loadpath;
    augeas *aug = NULL;
    struct RClass *rclass;
    mrb_value self;

    mrb_get_args(mrb, "z!z!i", &root, &loadpath, &flags);
    aug = aug_init(root, loadpath, flags);
    if (aug == NULL) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to initialize Augeas");
    }

    rclass = mrb_class_ptr(class);
    self = mrb_obj_new(mrb, rclass, 0, NULL);
    mrb_data_init(self, aug, &augeas_type);
    return self;
}

mrb_value
augeas_close (mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);

    aug_close(aug);
    mrb_data_init(self, NULL, &augeas_type);

    return mrb_nil_value();
}

static void
hash_set(mrb_state *mrb, mrb_value hash, const char *sym, mrb_value v) {
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_cstr(mrb, sym)), v);
}

/*
 * call-seq:
 *   error -> HASH
 *
 * Retrieve details about the last error encountered and return those
 * details in a HASH with the following entries:
 * - :code    error code from +aug_error+
 * - :message error message from +aug_error_message+
 * - :minor   minor error message from +aug_minor_error_message+
 * - :details error details from +aug_error_details+
 */
mrb_value
augeas_error(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    int code;
    const char *msg;
    mrb_value result;

    result = mrb_hash_new(mrb);

    code = aug_error(aug);
    hash_set(mrb, result, "code", mrb_fixnum_value(code));

    msg = aug_error_message(aug);
    if (msg != NULL)
        hash_set(mrb, result, "message", mrb_str_new_cstr(mrb, msg));

    msg = aug_error_minor_message(aug);
    if (msg != NULL)
        hash_set(mrb, result, "minor", mrb_str_new_cstr(mrb, msg));

    msg = aug_error_details(aug);
    if (msg != NULL)
        hash_set(mrb, result, "details", mrb_str_new_cstr(mrb, msg));

    return result;
}

static void
hash_set_range(mrb_state *mrb, mrb_value hash, const char *sym,
               unsigned int start, unsigned int end) {
    mrb_value r;

    r = mrb_range_new(mrb, mrb_fixnum_value(start), mrb_fixnum_value(end), 0);
    hash_set(mrb, hash, sym, r);
}

mrb_value
augeas_span(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *cpath;
    char *filename = NULL;
    unsigned int label_start, label_end, value_start, value_end,
        span_start, span_end;
    int r;
    mrb_value result;

    mrb_get_args(mrb, "z", &cpath);

    r = aug_span(aug, cpath,
                 &filename,
                 &label_start, &label_end,
                 &value_start, &value_end,
                 &span_start, &span_end);

    result = mrb_hash_new(mrb);

    if (r == 0) {
        hash_set(mrb, result, "filename", mrb_str_new_cstr(mrb, filename));
        hash_set_range(mrb, result, "label", label_start, label_end);
        hash_set_range(mrb, result, "value", value_start, value_end);
        hash_set_range(mrb, result, "span", span_start, span_end);
    }

    free(filename);

    return result;
}

/*
 * call-seq:
 *   label(PATH) -> String
 *
 * Lookup the label associated with PATH
 */
mrb_value
augeas_label(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *cpath;
    const char *label;

    mrb_get_args(mrb, "z", &cpath);

    aug_label(aug, cpath, &label);
    if (label != NULL) {
        return mrb_str_new_cstr(mrb, label);
    } else {
        return mrb_nil_value();
    }
}

/*
 * call-seq:
 *   rename(SRC, LABEL) -> int
 *
 * Rename the label of all nodes matching SRC to LABEL.
 *
 * Returns +false+ if +aug_rename+ fails, and the number of nodes renamed
 * on success.
 */
mrb_value
augeas_rename(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *csrc, *clabel;

    mrb_get_args(mrb, "zz", &csrc, &clabel);

    return mrb_bool_value(aug_rename(aug, csrc, clabel) == 0);
}

/*
 * call-seq:
 *   text_store(LENS, NODE, PATH) -> boolean
 *
 * Use the value of node NODE as a string and transform it into a tree
 * using the lens LENS and store it in the tree at PATH, which will be
 * overwritten. PATH and NODE are path expressions.
 */
mrb_value
augeas_text_store(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    char *clens, *cnode, *cpath;

    mrb_get_args(mrb, "zzz", &clens, &cnode, &cpath);

    return mrb_bool_value(aug_text_store(aug, clens, cnode, cpath) == 0);
}

/*
 * call-seq:
 *   text_retrieve(LENS, NODE_IN, PATH, NODE_OUT) -> boolean
 *
 * Transform the tree at PATH into a string using lens LENS and store it in
 * the node NODE_OUT, assuming the tree was initially generated using the
 * value of node NODE_IN. PATH, NODE_IN, and NODE_OUT are path expressions.
 */
mrb_value
augeas_text_retrieve(mrb_state *mrb, mrb_value self) {
    augeas *aug = aug_handle(mrb, self);
    const char *clens, *cnode_in, *cpath, *cnode_out;
    int r;

    mrb_get_args(mrb, "zzzz", &clens, &cnode_in, &cpath, &cnode_out);

    r = aug_text_retrieve(aug, clens, cnode_in, cpath, cnode_out);
    return mrb_bool_value(r == 0);
}

void
mrb_mruby_augeas_gem_init(mrb_state *mrb) {

    /* Define the ruby class */
    struct RClass* c_augeas = mrb_define_class(mrb, "Augeas", mrb->object_class);
    struct RClass* c_facade =
        mrb_define_class_under(mrb, c_augeas, "Facade", mrb->object_class);

    MRB_SET_INSTANCE_TT(c_augeas, MRB_TT_DATA);
    MRB_SET_INSTANCE_TT(c_facade, MRB_TT_DATA);

    /* Constants for enum aug_flags */
#define DEF_AUG_FLAG(name) \
    mrb_define_const(mrb, c_augeas, #name, mrb_fixnum_value(AUG_##name))
    DEF_AUG_FLAG(NONE);
    DEF_AUG_FLAG(SAVE_BACKUP);
    DEF_AUG_FLAG(SAVE_NEWFILE);
    DEF_AUG_FLAG(TYPE_CHECK);
    DEF_AUG_FLAG(NO_STDINC);
    DEF_AUG_FLAG(SAVE_NOOP);
    DEF_AUG_FLAG(NO_LOAD);
    DEF_AUG_FLAG(NO_MODL_AUTOLOAD);
    DEF_AUG_FLAG(ENABLE_SPAN);
#undef DEF_AUG_FLAG

    /* Constants for enum aug_errcode_t */
#define DEF_AUG_ERR(name) \
    mrb_define_const(mrb, c_augeas, #name, mrb_fixnum_value(AUG_##name))
    DEF_AUG_ERR(NOERROR);
    DEF_AUG_ERR(ENOMEM);
    DEF_AUG_ERR(EINTERNAL);
    DEF_AUG_ERR(EPATHX);
    DEF_AUG_ERR(ENOMATCH);
    DEF_AUG_ERR(EMMATCH);
    DEF_AUG_ERR(ESYNTAX);
    DEF_AUG_ERR(ENOLENS);
    DEF_AUG_ERR(EMXFM);
    DEF_AUG_ERR(ENOSPAN);
    DEF_AUG_ERR(EMVDESC);
    DEF_AUG_ERR(ECMDRUN);
    DEF_AUG_ERR(EBADARG);
    DEF_AUG_ERR(ELABEL);
#undef DEF_AUG_ERR

    /* Define the methods */
    mrb_define_class_method(mrb, c_augeas, "open3", augeas_init, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, c_augeas, "defvar", augeas_defvar, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, c_augeas, "defnode", augeas_defnode, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, c_augeas, "get", augeas_get, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, c_augeas, "exists", augeas_exists, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, c_augeas, "insert", augeas_insert, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, c_augeas, "mv", augeas_mv, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, c_augeas, "rm", augeas_rm, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, c_augeas, "match", augeas_match, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, c_augeas, "save", augeas_save, MRB_ARGS_NONE());
    mrb_define_method(mrb, c_augeas, "load", augeas_load, MRB_ARGS_NONE());
    mrb_define_method(mrb, c_augeas, "set_internal", augeas_set, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, c_augeas, "setm", augeas_setm, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, c_augeas, "close", augeas_close, MRB_ARGS_NONE());
    mrb_define_method(mrb, c_augeas, "error", augeas_error, MRB_ARGS_NONE());
    mrb_define_method(mrb, c_augeas, "span", augeas_span, MRB_ARGS_REQ(1));
    /* No binding since it requires a memfile */
    /* mrb_define_method(mrb, c_augeas, "srun", augeas_srun, MRB_ARGS_REQ(1)); */
    mrb_define_method(mrb, c_augeas, "label", augeas_label, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, c_augeas, "rename", augeas_rename, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, c_augeas, "text_store", augeas_text_store, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, c_augeas, "text_retrieve", augeas_text_retrieve, MRB_ARGS_REQ(4));

    /* Define methods to support the 'new' API in Augeas::Facade */
    mrb_define_class_method(mrb, c_facade, "open3", augeas_init, MRB_ARGS_REQ(3));
    /* The `close` and `error` methods as used unchanged in the ruby bindings */
    mrb_define_method(mrb, c_facade, "close", augeas_close, MRB_ARGS_NONE());
    mrb_define_method(mrb, c_facade, "error", augeas_error, MRB_ARGS_NONE());
    mrb_define_method(mrb, c_facade, "augeas_defvar", augeas_defvar, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, c_facade, "augeas_defnode", augeas_defnode, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, c_facade, "augeas_get", augeas_get, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, c_facade, "augeas_exists", augeas_exists, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, c_facade, "augeas_insert", augeas_insert, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, c_facade, "augeas_mv", augeas_mv, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, c_facade, "augeas_rm", augeas_rm, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, c_facade, "augeas_match", facade_match, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, c_facade, "augeas_save", facade_save, MRB_ARGS_NONE());
    mrb_define_method(mrb, c_facade, "augeas_load", augeas_load, MRB_ARGS_NONE());
    mrb_define_method(mrb, c_facade, "augeas_set", facade_set, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, c_facade, "augeas_setm", augeas_setm, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, c_facade, "augeas_span", augeas_span, MRB_ARGS_REQ(1));
    /* No binding since it requires a memfile */
    /* mrb_define_method(mrb, c_facade, "augeas_srun", augeas_srun, MRB_ARGS_REQ(1)); */
    mrb_define_method(mrb, c_facade, "augeas_label", augeas_label, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, c_facade, "augeas_rename", augeas_rename, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, c_facade, "augeas_text_store", augeas_text_store, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, c_facade, "augeas_text_retrieve", augeas_text_retrieve, MRB_ARGS_REQ(4));
}

void
mrb_mruby_augeas_gem_final(mrb_state *mrb) {
}
