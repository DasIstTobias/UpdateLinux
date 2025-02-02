/*
 * update_program.c
 *
 * A simple Linux update GUI program using GTK3.
 * It displays system information (CPU, GPU, RAM, and distro)
 * and an Update button (styled as a circle) in a gradient background.
 * When clicked, it runs a Bash update script and updates status as the script prints output.
 *
 * Compile with:
 *    gcc update_program.c -o update_program `pkg-config --cflags --libs gtk+-3.0`
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define UPDATE_SCRIPT "./update_script.sh"

/* Structure to pass information for updating status from the worker thread */
typedef struct {
    GtkLabel *status_label;
    gchar *text;
} UpdateStatusData;

/* Callback to update the status label text on the GTK main thread */
gboolean update_status_cb(gpointer user_data) {
    UpdateStatusData *ud = (UpdateStatusData *)user_data;
    gtk_label_set_text(ud->status_label, ud->text);
    g_free(ud->text);
    g_free(ud);
    return FALSE; // remove source after execution
}

/* Helper: Get CPU model name from /proc/cpuinfo */
char *get_cpu_info() {
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp)
        return g_strdup("CPU: Unknown");
    char line[256];
    char *model = NULL;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "model name")) {
            char *colon = strchr(line, ':');
            if (colon) {
                model = g_strdup_printf("CPU: %s", colon + 2); // skip ": "
                model[strcspn(model, "\n")] = '\0';
                break;
            }
        }
    }
    fclose(fp);
    if (!model)
        return g_strdup("CPU: Unknown");
    return model;
}

/* Helper: Get RAM total from /proc/meminfo and convert it to Gigabytes */
char *get_ram_info() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp)
        return g_strdup("RAM: Unknown");
    char line[256];
    unsigned long mem_kb = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "MemTotal")) {
            sscanf(line, "MemTotal: %lu kB", &mem_kb);
            break;
        }
    }
    fclose(fp);
    if (!mem_kb)
        return g_strdup("RAM: Unknown");
    double mem_gb = mem_kb / 1048576.0;  // 1 GB = 1048576 kB
    return g_strdup_printf("RAM: %.2f GB", mem_gb);
}

/* Helper: Get GPU info by running lspci and extracting the human-readable name */
char *get_gpu_info() {
    FILE *fp = popen("lspci | grep -i 'vga'", "r");
    if (!fp)
        return g_strdup("GPU: Unknown");
    char buffer[256];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        pclose(fp);
        return g_strdup("GPU: Unknown");
    }
    pclose(fp);
    char *marker = strstr(buffer, "VGA compatible controller:");
    if (marker) {
        marker += strlen("VGA compatible controller:");
        while (*marker == ' ')
            marker++;
        char *paren = strchr(marker, '(');
        if (paren)
            *paren = '\0';
        marker[strcspn(marker, "\n")] = '\0';
        return g_strdup_printf("GPU: %s", marker);
    } else {
        buffer[strcspn(buffer, "\n")] = '\0';
        return g_strdup_printf("GPU: %s", buffer);
    }
}

/* Helper: Get Linux Distribution info from /etc/os-release */
char *get_distro_info() {
    FILE *fp = fopen("/etc/os-release", "r");
    if (!fp)
        return g_strdup("Distro: Unknown");
    char line[256];
    char *distro = NULL;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *start = strchr(line, '=');
            if (start) {
                start++;
                if (*start == '\"')
                    start++;
                char *end = strchr(start, '\"');
                if (end)
                    *end = '\0';
                distro = g_strdup_printf("Distro: %s", start);
                break;
            }
        }
    }
    fclose(fp);
    if (!distro)
        return g_strdup("Distro: Unknown");
    return distro;
}

/* Worker thread function to run the update script and capture its output */
void *run_update_script(void *arg) {
    GtkLabel *status_label = (GtkLabel *)arg;
    FILE *fp = popen(UPDATE_SCRIPT, "r");
    if (!fp) {
        UpdateStatusData *ud = g_malloc(sizeof(UpdateStatusData));
        ud->status_label = status_label;
        ud->text = g_strdup("Error: Could not run update script.");
        g_idle_add(update_status_cb, ud);
        return NULL;
    }
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        UpdateStatusData *ud = g_malloc(sizeof(UpdateStatusData));
        ud->status_label = status_label;
        ud->text = g_strdup(buffer);
        g_idle_add(update_status_cb, ud);
        g_usleep(300000);  // 0.3 seconds
    }
    pclose(fp);
    return NULL;
}

/* Callback when the Update button is clicked */
static void on_update_clicked(GtkButton *button, gpointer user_data) {
    GtkLabel *status_label = GTK_LABEL(user_data);
    gtk_label_set_text(status_label, "Starting update...");
    pthread_t tid;
    pthread_create(&tid, NULL, run_update_script, status_label);
    pthread_detach(tid);
}

/* Main: Set up the GTK window and layout */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    /* Create the main window */
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Linux Update Program");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Create an overlay container for the gradient background */
    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), overlay);

    /* Create a main grid for layout */
    GtkWidget *main_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(main_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(main_grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_grid), 10);
    gtk_container_add(GTK_CONTAINER(overlay), main_grid);

    /* Retrieve system information */
    char *cpu = get_cpu_info();
    char *ram = get_ram_info();
    char *gpu = get_gpu_info();
    char *distro = get_distro_info();

    /* Create left system info label (CPU and RAM) */
    gchar *left_text = g_strdup_printf("%s\n\n%s", cpu, ram);
    GtkWidget *label_left = gtk_label_new(left_text);
    gtk_label_set_line_wrap(GTK_LABEL(label_left), TRUE);
    gtk_widget_set_valign(label_left, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(label_left, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label_left, TRUE);
    g_free(cpu);
    g_free(ram);
    g_free(left_text);

    /* Create right system info label (GPU and Distro) */
    gchar *right_text = g_strdup_printf("%s\n\n%s", gpu, distro);
    GtkWidget *label_right = gtk_label_new(right_text);
    gtk_label_set_line_wrap(GTK_LABEL(label_right), TRUE);
    gtk_widget_set_valign(label_right, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(label_right, GTK_ALIGN_END);
    gtk_widget_set_hexpand(label_right, TRUE);
    g_free(gpu);
    g_free(distro);
    g_free(right_text);

    /* Create the center update button (styled as a circle) */
    GtkWidget *update_button = gtk_button_new_with_label("Update");
    /* Remove any fixed size so it scales. */
    gtk_widget_set_hexpand(update_button, TRUE);
    gtk_widget_set_vexpand(update_button, TRUE);
    gtk_widget_set_halign(update_button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(update_button, GTK_ALIGN_CENTER);

    /* Apply CSS styling with turquoise background and circular shape */
    GtkCssProvider *css_provider = gtk_css_provider_new();
    const gchar *css_data =
        "button {"
        "  border-radius: 50%;"
        "  background: turquoise;"
        "  color: white;"
        "  font-size: 2em;"
        "  font-weight: bold;"
        "  padding: 10px;"
        "}"
        "button:hover {"
        "  background: darkturquoise;"
        "}";
    gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
    GtkStyleContext *btn_context = gtk_widget_get_style_context(update_button);
    gtk_style_context_add_provider(btn_context,
                                   GTK_STYLE_PROVIDER(css_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(css_provider);

    /* Wrap the update button in an Aspect Frame to force a square (1:1 aspect ratio)
       and hide its border so only the circular button is visible */
    GtkWidget *aspect_frame = gtk_aspect_frame_new(NULL, 0.5, 0.5, 1.0, FALSE);
    gtk_frame_set_shadow_type(GTK_FRAME(aspect_frame), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(aspect_frame), update_button);

    /* Create a status label at the bottom to show update messages */
    GtkWidget *status_label = gtk_label_new("Status: Idle");
    gtk_label_set_line_wrap(GTK_LABEL(status_label), TRUE);
    gtk_widget_set_valign(status_label, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(status_label, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(status_label, TRUE);

    /* Create a top grid with three columns to hold left label, the aspect frame (with update button), and right label */
    GtkWidget *top_grid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(top_grid), TRUE);
    gtk_grid_set_column_spacing(GTK_GRID(top_grid), 10);
    gtk_grid_attach(GTK_GRID(top_grid), label_left, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(top_grid), aspect_frame, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(top_grid), label_right, 2, 0, 1, 1);

    /* Allow the top grid to expand */
    gtk_widget_set_hexpand(top_grid, TRUE);
    gtk_widget_set_vexpand(top_grid, TRUE);

    /* Attach the top grid (first row) and the status label (second row) to the main grid */
    gtk_grid_attach(GTK_GRID(main_grid), top_grid, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), status_label, 0, 1, 1, 1);

    /* Connect the update button click signal */
    g_signal_connect(update_button, "clicked", G_CALLBACK(on_update_clicked), status_label);

    /* Set a CSS provider on the overlay to add a gradient background */
    GtkCssProvider *css_bg = gtk_css_provider_new();
    const gchar *css_bg_data =
        "window {"
        "  background-image: linear-gradient(to right, orange, blue);"
        "}";
    gtk_css_provider_load_from_data(css_bg, css_bg_data, -1, NULL);
    GtkStyleContext *win_context = gtk_widget_get_style_context(window);
    gtk_style_context_add_provider(win_context,
                                   GTK_STYLE_PROVIDER(css_bg),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_bg);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
