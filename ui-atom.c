/* ui-atom.c: functions for atom feeds
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-atom.h"
#include "html.h"
#include "ui-shared.h"
#include "ui-diff.h"

static void add_entry(struct commit *commit, struct commitinfo *info, const char *host, int enable_atom_diff)
{
	char delim = '&';
	char *hex;
	char *hex_parent;
	char *mail, *t, *t2;

	hex = sha1_to_hex(commit->object.sha1);
	if (commit->parents) {
		hex_parent = sha1_to_hex(commit->parents->item->object.sha1);
	} else {
		hex_parent = NULL; /* means before initial commit */
	}
	html("<entry>\n");
	html("<title>");
	html_txt(info->subject);
	html("</title>\n");
	html("<updated>");
	cgit_print_date(info->committer_date, FMT_ATOMDATE, 0);
	html("</updated>\n");
	html("<author>\n");
	if (info->author) {
		html("<name>");
		html_txt(info->author);
		html("</name>\n");
	}
	if (info->author_email && !ctx.cfg.noplainemail) {
		mail = xstrdup(info->author_email);
		t = strchr(mail, '<');
		if (t)
			t++;
		else
			t = mail;
		t2 = strchr(t, '>');
		if (t2)
			*t2 = '\0';
		html("<email>");
		html_txt(t);
		html("</email>\n");
		free(mail);
	}
	html("</author>\n");
	html("<published>");
	cgit_print_date(info->author_date, FMT_ATOMDATE, 0);
	html("</published>\n");
	if (host) {
		html("<link rel='alternate' type='text/html' href='");
		html(cgit_httpscheme());
		html_attr(host);
		html_attr(cgit_pageurl(ctx.repo->url, "commit", NULL));
		if (ctx.cfg.virtual_root)
			delim = '?';
		htmlf("%cid=%s", delim, hex);
		html("'/>\n");

		html("<id>");
		html(cgit_httpscheme());
		html_attr(host);
		html_attr(cgit_repourl(ctx.repo->url));
		htmlf("/commit/?id=%s</id>\n", hex);
	} else {
		htmlf("<id>urn:tag:%s</id>\n", hex);
	}
	html("<content type='xhtml'>\n");
	html("<div xmlns='http://www.w3.org/1999/xhtml'>\n");
	html("<pre>\n");
	html_txt(info->msg);
	html("</pre>\n");
	if (enable_atom_diff) {
		html("<pre class='diff'>\n");
		html("<style scoped>\n"); /* HTML5 with graceful degradation */
		html("table.diff .add, span.add { background-color: #afa; }");
		html("table.diff .del, span.remove { background-color: #faa; }");
		html("</style>\n");
		cgit_print_diff(hex, hex_parent, NULL);
		html("</pre>");
	}
	html("</div>\n");
	html("</content>\n");
	html("</entry>\n");
}


void cgit_print_atom(char *tip, char *path, int max_count, int enable_atom_diff)
{
	const char *host;
	const char *argv[] = {NULL, tip, NULL, NULL, NULL};
	struct commit *commit;
	struct rev_info rev;
	int argc = 2;
	int had_global_updated = 0;

	if (ctx.qry.show_all)
		argv[1] = "--all";
	else if (!tip)
		argv[1] = ctx.qry.head;

	if (path) {
		argv[argc++] = "--";
		argv[argc++] = path;
	}

	init_revisions(&rev, NULL);
	rev.abbrev = DEFAULT_ABBREV;
	rev.commit_format = CMIT_FMT_DEFAULT;
	rev.verbose_header = 1;
	rev.show_root_diff = 0;
	rev.max_count = max_count;
	setup_revisions(argc, argv, &rev, NULL);
	prepare_revision_walk(&rev);

	host = cgit_hosturl();
	ctx.page.mimetype = "text/xml";
	ctx.page.charset = "utf-8";
	cgit_print_http_headers();
	html("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	html("<feed xmlns='http://www.w3.org/2005/Atom'>\n");
	html("<title>");
	html_txt(ctx.repo->name);
	if (path) {
		html("/");
		html_txt(path);
	}
	if (tip && !ctx.qry.show_all) {
		html(", branch ");
		html_txt(tip);
	}
	html("</title>\n");
	html("<subtitle>");
	html_txt(ctx.repo->desc);
	html("</subtitle>\n");
	if (host) {
		html("<link rel='alternate' type='text/html' href='");
		html(cgit_httpscheme());
		html_attr(host);
		html_attr(cgit_repourl(ctx.repo->url));
		html("'/>\n");

		html("<id>");
		html(cgit_httpscheme());
		html_txt(host);
		html_txt(cgit_repourl(ctx.repo->url));
		html("</id>\n");
	}

	while ((commit = get_revision(&rev)) != NULL) {
		struct commitinfo *info = cgit_parse_commit(commit);
		if (!had_global_updated) {
			html("<updated>");
			cgit_print_date(info->committer_date, FMT_ATOMDATE, 0);
			html("</updated>\n");
			had_global_updated = 1;
		}
		add_entry(commit, info, host, enable_atom_diff);

		cgit_free_commitinfo(info);
		free_commit_buffer(commit);
		free_commit_list(commit->parents);
		commit->parents = NULL;
	}
	html("</feed>\n");
}
