/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifdef CONFIG_H
#include <config.h>
#endif
#include "trimodel.h"

#include "fate.h"

#define CMP(x, y) (((x) > (y)) - ((x) < (y)))

int
trimodel_element_cmp_pair (const TrimodelElement *elt,
                           const Token           *t1,
                           const Token           *t2)
{
    guint x_id, y_id;

    x_id = GPOINTER_TO_UINT (elt->t1);
    y_id = GPOINTER_TO_UINT (t1);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = GPOINTER_TO_UINT (elt->t2);
    y_id = GPOINTER_TO_UINT (t2);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    return 0;
}

int
trimodel_element_cmp (gconstpointer x_ptr,
                      gconstpointer y_ptr)
{
    const TrimodelElement *x = x_ptr;
    const TrimodelElement *y = y_ptr;
    guint x_id, y_id;

    x_id = GPOINTER_TO_UINT (x->t1);
    y_id = GPOINTER_TO_UINT (y->t1);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = GPOINTER_TO_UINT (x->t2);
    y_id = GPOINTER_TO_UINT (y->t2);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = token_get_syllables (x->soln);
    y_id = token_get_syllables (y->soln);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = GPOINTER_TO_UINT (x->soln);
    y_id = GPOINTER_TO_UINT (y->soln);
    return CMP (x_id, y_id);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

Trimodel *
trimodel_new (void)
{
    Trimodel *tri;

    tri = g_new0 (Trimodel, 1);
    REFCOUNT_INIT (tri);
    PYBIND_INIT (tri);

    tri->array_AB_C = g_array_new (FALSE, FALSE, sizeof (TrimodelElement));
    tri->array_AC_B = g_array_new (FALSE, FALSE, sizeof (TrimodelElement));
    tri->array_BC_A = g_array_new (FALSE, FALSE, sizeof (TrimodelElement));

    tri->is_sorted = FALSE;

    return tri;
}

static void
trimodel_dealloc (Trimodel *tri)
{
    g_return_if_fail (tri != NULL);

    g_array_free (tri->array_AB_C, TRUE);
    g_array_free (tri->array_AC_B, TRUE);
    g_array_free (tri->array_BC_A, TRUE);
    
    g_list_foreach (tri->text_list, (GFunc) text_unref, NULL);
    g_list_free (tri->text_list);
    
    g_free (tri);
}

static void
trimodel_sort_single_array (Trimodel *tri,
                            GArray   *array)
{
    int i, first_i, firstsyl_i, syl_n;
    TrimodelElement *elt, *first, *firstsyl;

    if (array->len < 2)
        return;

    g_array_sort (array, trimodel_element_cmp);

    /* Attach offset information. */
    first_i = firstsyl_i = 0;
    first = firstsyl = &g_array_index (array, TrimodelElement, 0);
    syl_n = -1;

    for (i = 1; i < array->len; ++i) {
        elt = &g_array_index (array, TrimodelElement, i);

        if (first->t1 == elt->t1 && first->t2 == elt->t2) {
            first->key_offset = i - first_i;
            elt->key_offset = first_i - i;

            if (syl_n == token_get_syllables (elt->soln)) {
                firstsyl->syl_offset = i - firstsyl_i;
                elt->syl_offset = firstsyl_i - i;
            } else {
                firstsyl_i = i;
                firstsyl = elt;
                firstsyl->syl_offset = 0;
                syl_n = token_get_syllables (firstsyl->soln);
            }

        } else {
            first_i = firstsyl_i = i;
            first = firstsyl = elt;
            first->key_offset = 0;
            firstsyl->syl_offset = 0;
        }
    }
}

static void
trimodel_do_sort (Trimodel *tri)
{
    g_return_if_fail (tri != NULL);

    if (! tri->is_sorted) {

        trimodel_sort_single_array (tri, tri->array_AB_C);
        trimodel_sort_single_array (tri, tri->array_AC_B);
        trimodel_sort_single_array (tri, tri->array_BC_A);

        tri->is_sorted = TRUE;
    }
}

static void
trimodel_add_triple (Trimodel *tri,
                     Token    *a,
                     Token    *b,
                     Token    *c)
{
    TrimodelElement elt;

    g_return_if_fail (tri != NULL);
    g_return_if_fail (a != NULL);
    g_return_if_fail (b != NULL);
    g_return_if_fail (c != NULL);

    elt.key_offset = 0;
    elt.syl_offset = 0;
    
    elt.t1 = a;
    elt.t2 = b;
    elt.soln = c;
    g_array_append_val (tri->array_AB_C, elt);

    elt.t1 = a;
    elt.t2 = c;
    elt.soln = b;
    g_array_append_val (tri->array_AC_B, elt);

    elt.t1 = b;
    elt.t2 = c;
    elt.soln = a;
    g_array_append_val (tri->array_BC_A, elt);

    tri->is_sorted = FALSE;
}

void
trimodel_add_text (Trimodel *tri,
                   Text     *txt)
{
    Token *brk, *tok, *window[3];
    int i, j, N;

    g_return_if_fail (tri != NULL);
    g_return_if_fail (txt != NULL);

    brk = token_lookup_break ();
    g_assert (brk != NULL);

    window[0] = brk;
    window[1] = brk;
    window[2] = brk;
    
    N = text_get_length (txt);
    for (i = 0; i < N; ++i) {
        tok = text_get_token (txt, i);

        j = 0;
        do {
            window[0] = window[1];
            window[1] = window[2];
            window[2] = tok;
            trimodel_add_triple (tri, window[0], window[1], window[2]);
            ++j;
        } while (j < 2 && token_is_break (tok));
    }

    tri->text_list = g_list_append (tri->text_list, text_ref (txt));
}

void
trimodel_prepare (Trimodel *tri)
{
    g_return_if_fail (tri != NULL);
    trimodel_do_sort (tri);
}

static gboolean
trimodel_query_array (Trimodel *tri,
                      GArray   *array,
                      Token    *t1,
                      Token    *t2,
                      int       min_syllables,
                      int       max_syllables,
                      int      *out_i0,
                      int      *out_i1)
{
    int a, b, i, i0, i1, cmp;
    const TrimodelElement *elt;

    g_return_val_if_fail (tri != NULL, FALSE);
    g_return_val_if_fail (t1 != NULL, FALSE);
    g_return_val_if_fail (t2 != NULL, FALSE);

    if (min_syllables >= 0 
        && max_syllables >= 0 
        && min_syllables > max_syllables)
        return FALSE;

    trimodel_do_sort (tri);

    /* First, find a possible hit. */

    a = 0;
    b = array->len-1;
    i = -1;

    while (b-a > 1) {
        i = (a+b)/2;

        elt = &g_array_index (array, TrimodelElement, i);
        cmp = trimodel_element_cmp_pair (elt, t1, t2);

        if (cmp == 0)
            break;
        else if (cmp < 0)
            a = i;
        else /* cmp > 0 */
            b = i;

        i = -1;
    }

    if (b-a <= 1 && i != -1) {
        elt = &g_array_index (array, TrimodelElement, a);
        cmp = trimodel_element_cmp_pair (elt, t1, t2);
        if (cmp == 0)
            i = a;
        else if (a != b) {
            elt = &g_array_index (array, TrimodelElement, b);
            cmp = trimodel_element_cmp_pair (elt, t1, t2);
            if (cmp == 0)
                i = b;
        }
    }

    if (i == -1)
        return FALSE;


    /* Next, use the hit to find the beginning and end of our
       solution range. */
    g_assert (0 <= i && i < array->len);
    elt = &g_array_index (array, TrimodelElement, i);
    g_assert (elt->t1 == t1);
    g_assert (elt->t2 == t2);

    i0 = i;
    if (elt->key_offset < 0) /* move back to start */
        i0 += elt->key_offset;

    g_assert (0 <= i && i < array->len);
    elt = &g_array_index (array, TrimodelElement, i0);
    g_assert (elt->key_offset >= 0);
    g_assert (elt->t1 == t1);
    g_assert (elt->t2 == t2);

    i1 = i0 + elt->key_offset;

    /* Adjust bounds to find solutions w/ right numbers of syllables. */
    if (min_syllables >= 0) {
        while (i0 <= i1) {
            elt = &g_array_index (array, TrimodelElement, i0);
            if (min_syllables <= token_get_syllables (elt->soln)) {
                /* If min_syllables == max_syllables, we have
                   enough information to skip the separate max_syllables
                   check. */
                if (min_syllables == max_syllables) {
                    i1 = i0 + elt->syl_offset;
                    max_syllables = -1;
                }
                break;
            }
            i0 += elt->syl_offset+1;
        }
    }

    if (max_syllables >= 0) {
        while (i0 <= i1) {
            elt = &g_array_index (array, TrimodelElement, i0);
            if (token_get_syllables (elt->soln) <= max_syllables)
                break;
            i1 -= elt->syl_offset+1;
        }
    }

    if (i0 > i1)
        return FALSE;

    if (out_i0)
        *out_i0 = i0;

    if (out_i1)
        *out_i1 = i1;
    
    return TRUE;
}

gint
trimodel_query (Trimodel    *tri,
                Token       *token_a,
                Token       *token_b,
                Token       *token_c,
                TokenFilter *filter,
                TokenFn      match_fn,
                gpointer     user_data)
{

    int i0, i1, i, count;
    int wild_count;
    Token *t1 = NULL, *t2 = NULL;
    GArray *array = NULL;
    const TrimodelElement *elt;
    Token *prev_soln;
    gboolean ok;

    g_return_val_if_fail (tri != NULL, -1);
    g_return_val_if_fail (token_a != NULL, -1);
    g_return_val_if_fail (token_b != NULL, -1);
    g_return_val_if_fail (token_c != NULL, -1);

    wild_count = 0;
    if (token_is_wildcard (token_a))
        ++wild_count;
    if (token_is_wildcard (token_b))
        ++wild_count;
    if (token_is_wildcard (token_c))
        ++wild_count;
    if (wild_count != 1) {
        g_warning ("ignoring ill-formed query w/ %d wildcards", wild_count);
        return -1;
    }

    if (token_is_wildcard (token_a)) {
        t1 = token_b;
        t2 = token_c;
        array = tri->array_BC_A;
    } else if (token_is_wildcard (token_b)) {
        t1 = token_a;
        t2 = token_c;
        array = tri->array_AC_B;
    } else if (token_is_wildcard (token_c)) {
        t1 = token_a;
        t2 = token_b;
        array = tri->array_AB_C;
    } else {
        g_assert_not_reached ();
    }

    g_assert (t1 != NULL);
    g_assert (t2 != NULL);
    g_assert (array != NULL);

    if (! trimodel_query_array (tri,
                                array,
                                t1, t2,
                                filter ? filter->min_syllables : -1,
                                filter ? filter->max_syllables : -1,
                                &i0, &i1)) {
        return 0;
    }

    g_assert (i0 <= i1);

    prev_soln = NULL;
    ok = FALSE;
    count = 0;
    for (i = i0; i <= i1; ++i) {
        elt = &g_array_index (array, TrimodelElement, i);
        if (filter == NULL) {
            ok = TRUE;
        } else if (elt->soln == prev_soln) {
            /* retain previous value of ok */
        } else {
            ok = token_filter_test (filter, elt->soln);
            prev_soln = elt->soln;
        }

        if (ok) {
            if (match_fn)
                match_fn (elt->soln, user_data);
            ++count;
        }
    }

    return count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

REFCOUNT_CODE (Trimodel, trimodel);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PYBIND_CODE (Trimodel, trimodel);

static PyObject *
py_trimodel_add_text (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    PyObject *py_txt;
    Text *txt;
    
    if (! (PyArg_ParseTuple (args, "O", &py_txt) && py_text_check (py_txt)))
        return NULL;

    txt = text_from_py (py_txt);
    trimodel_add_text (tri, txt);

    Py_INCREF (Py_None);
    return Py_None;
}

static PyObject *
py_trimodel_prepare (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    trimodel_prepare (tri);

    Py_INCREF (Py_None);
    return Py_None;
}

static gboolean
query_token_cb (Token *t, gpointer user_data)
{
    GPtrArray *results_array = user_data;
    g_ptr_array_add (results_array, t);
    return TRUE;
}

static PyObject *
py_trimodel_query (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    PyObject *py_t1, *py_t2, *py_t3;
    Token *t1, *t2, *t3;
    PyObject *py_filter;
    TokenFilter filter;
    
    GPtrArray *results_array;
    GHashTable *uniq = NULL;
    PyObject *results = NULL;
    int i, j, N;

    if (! PyArg_ParseTuple (args, "OOOO",
                            &py_t1, &py_t2, &py_t3, &py_filter))
        return NULL;

    t1 = token_from_py (py_t1);
    t2 = token_from_py (py_t2);
    t3 = token_from_py (py_t3);

    token_filter_init_from_py_dict (&filter, py_filter);

    results_array = g_ptr_array_sized_new (500);

    if (! trimodel_query (tri,
                          t1, t2, t3, &filter,
                          query_token_cb, results_array)) {
        goto finished;
    }

    /* Shuffle results */
    N = results_array->len;
    for (i = 0; i < N-1; ++i) {
        j = i + fate_random (N-i);
        if (i != j) {
            Token *tmp = g_ptr_array_index (results_array, i);
            g_ptr_array_index (results_array, i) = 
                g_ptr_array_index (results_array, j);
            g_ptr_array_index (results_array, j) = tmp;
        }
    }

    /* Uniqueify array */
    results = PyList_New (0);
    uniq = g_hash_table_new (NULL, NULL);
    for (i = 0; i < N; ++i) {
        Token *tok = g_ptr_array_index (results_array, i);
        if (g_hash_table_lookup (uniq, tok) == NULL) {
            PyObject *py_tok = token_to_py (tok);
            PyList_Append (results, py_tok);
            Py_DECREF (py_tok);
            g_hash_table_insert (uniq, tok, tok);
        }
    }

 finished:

    if (results_array != NULL)
        g_ptr_array_free (results_array, TRUE);

    if (uniq != NULL)
        g_hash_table_destroy (uniq);

    if (results == NULL)
        results = PyList_New (0);

    return results;
}

static PyMethodDef py_trimodel_methods[] = {
    { "add_text", py_trimodel_add_text, METH_VARARGS },
    { "prepare",  py_trimodel_prepare,  METH_NOARGS  },
    { "query",    py_trimodel_query,    METH_VARARGS },

    { NULL, NULL, 0 }

};

static Trimodel *
py_trimodel_assemble (PyObject *args, PyObject *kwdict)
{
    return trimodel_new ();
}

void
py_trimodel_register (PyObject *dict)
{
    PYBIND_REGISTER_CODE (Trimodel, trimodel, dict);
}