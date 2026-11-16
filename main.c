#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

struct Playlist {
  char name[256];
  char **tracks;		/* paths to tracks */
  uint32_t tracks_len;
  bool is_playing;
};

struct Playlist *create_playlist(char *playlist_dir_path) {
  struct Playlist *p = malloc(sizeof(struct Playlist));
  p->is_playing = false;
  p->tracks = NULL;
  strncpy(p->name, playlist_dir_path, sizeof(p->name));

  // TODO: traverse directory for DT_REG files, and add tracks
}

void free_playlist(struct Playlist *p) {
  if (p->tracks != NULL) {
    for (int i = 0; i < p->tracks_len; i++) {
      free(p->tracks[0]);
    }
  }
}

struct Playlist *playlists;
// globally store an array of playlists

/* Returns -1 if failure */
int load_playlists(char *tracks_dir_name) {
  DIR *tracks_dir = opendir(tracks_dir_name);
  if (!tracks_dir) {
    return -1; 
  }

  char curr_dir_name[256];
  curr_dir_name = getcwd(curr_dir_name, sizeof(curr_dir_name));
  if (curr_dir_name) {
    printf("Current dir directory: %s\n", curr_dir_name);
  }
  
  struct dirent *item;
  while ((item = readdir(tracks_dir)) != NULL) {
    bool is_directory = item->d_type == DT_DIR &&
      strstr(item->d_name, ".") == NULL;
    if (is_directory) {
      printf("creating playlist from directory: %s\n", item->d_name);
    }
  }

  closedir(tracks_dir);
}

int main(int argc, char *argv[]) {
  int r;
  
  r = load_playlists("tracks");
  if (r == -1) {
    return -1;
  }

  return 0;
  
  ma_result result;
  ma_engine engine;
  result = ma_engine_init(NULL, &engine);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "Unable to init engine\n");
    return -1;
  }

  ma_sound focus_track;
  result = ma_sound_init_from_file(&engine, "focus.mp3", 0, NULL, NULL, &focus_track);

  ma_sound_start(&focus_track);

  while (ma_sound_is_playing(&focus_track)) {
  }

  ma_sound_uninit(&focus_track);
  ma_engine_uninit(&engine);
}
