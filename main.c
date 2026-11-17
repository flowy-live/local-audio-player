#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

struct Playlist {
  char name[256];
  
  char **tracks;		/* relative paths to tracks */
  size_t tracks_len;
  size_t tracks_capacity;

  bool is_playing;
};

void free_playlist(struct Playlist *p) {
  if (p->tracks) {
    for (int i = 0; i < p->tracks_len; i++) {
      free(p->tracks[i]);
    }

    free(p->tracks);
  }
}

int init_playlist(char *path, struct Playlist *p) {
  p->is_playing = false;
  
  p->tracks_len = 0;
  p->tracks_capacity = 10;
  p->tracks = calloc(p->tracks_capacity, sizeof(char *));
  if (!p->tracks) return -1;

  char *base = strrchr(path, '/');
  base = base ? base + 1 : path;
  strncpy(p->name, base, sizeof(p->name) - 1);
  p->name[sizeof(p->name) - 1] = '\0';

  DIR *dir;
  dir = opendir(path);
  if (!dir) { free_playlist(p); return -1; }
  
  struct dirent *item;
  while ((item = readdir(dir)) != NULL) {
    if (item->d_type == DT_REG) {
      if (p->tracks_capacity == p->tracks_len) {
	size_t new_capacity = p->tracks_capacity * 2;
	char **tmp = reallocarray(p->tracks, new_capacity, sizeof(char *));
	if (!tmp) {
	  free_playlist(p);
	  closedir(dir);
	  return -1;
	}

	p->tracks = tmp;
	p->tracks_capacity = new_capacity;
      }

      size_t track_len = strlen(item->d_name) + strlen(path) + 2; // '/' and '\0'
      char *track = malloc(sizeof(*track) * (track_len));
      if (!track) {
	free_playlist(p);
	closedir(dir);
	return -1;
      }
      
      snprintf(track, track_len, "%s/%s", path, item->d_name);
      p->tracks[p->tracks_len++] = track;
    }
  }

  closedir(dir);
  return 0;
}

/* Returns -1 if failure */
int load_playlists(const char *playlists_path, struct Playlist **out, size_t *len_out) {
  DIR *dir;
  char curr_dir_name[256];
  struct dirent *item;
  
  dir = opendir(playlists_path);
  if (!dir) {
    return -1; 
  }

  getcwd(curr_dir_name, sizeof(curr_dir_name));
  printf("Current dir directory: %s\n", curr_dir_name);

  size_t len = 0, cap = 8;
  struct Playlist *arr = calloc(cap, sizeof(*arr));
  if (!arr) {
    closedir(dir);
    return -1;
  }

  while ((item = readdir(dir)) != NULL) {
    bool is_directory = item->d_type == DT_DIR &&
      strstr(item->d_name, ".") == NULL;
    
    if (is_directory) {
      char path[1024];
      snprintf(path, sizeof(path), "%s/%s", playlists_path, item->d_name);
      
      if (len == cap) {
	struct Playlist *tmp = reallocarray(arr, cap * 2, sizeof(*arr));
	if (!tmp) {
	  goto fail;
	}
	arr = tmp;
	cap *= 2;
      }
      
      int r = init_playlist(path, &arr[len]);
      if (r == -1) goto fail;
      len++;
    }
  }

  closedir(dir);
  *len_out = len;
  *out = arr;
  return 0;
  
 fail:
  for (size_t i = 0; i < len; i++) free_playlist(&arr[i]);
  free(arr);
  closedir(dir);
  return -1;
}

void free_playlists(struct Playlist *playlists, size_t len) {
  for (size_t i = 0; i < len; i++) free_playlist(&playlists[i]);
  free(playlists);
}

int main(int argc, char *argv[]) {
  int r;
  struct Playlist *playlists;
  size_t len_playlists;

  if (argc < 2) return -1;
  
  r = load_playlists(argv[1], &playlists, &len_playlists);
  if (r == -1) {
    fprintf(stderr, "Unable to load playlists\n");
    return -1;
  }

  for (int i = 0; i < len_playlists; i++) {
    printf("> Playlist: %s\n", playlists[i].name);

    for (int t = 0; t < playlists[i].tracks_len; t++) {
      printf(">>> Track: %s\n", playlists[i].tracks[t]);
    }
  }

  char selected_playlist[1024];
  char *sel = fgets(selected_playlist, sizeof(selected_playlist), stdin);
  if (!sel) {
    free_playlists(playlists, len_playlists);
    return -1;
  }

  selected_playlist[strcspn(selected_playlist, "\n")] = '\0';
  printf("Beginning to play: %s\n", selected_playlist);

  ma_result result;
  ma_engine engine;
  result = ma_engine_init(NULL, &engine);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "Unable to init engine\n");
    free_playlists(playlists, len_playlists);
    return -1;
  }

  struct Playlist *curr_playlist = NULL;
  for (int i = 0; i < len_playlists; i++) {
    if (strstr(playlists[i].name, selected_playlist) != NULL) {
      curr_playlist = &playlists[i];
      break;
    }
  }
  if (!curr_playlist) {
    fprintf(stderr, "Playlist not found!\n");
    free_playlists(playlists, len_playlists);
    ma_engine_uninit(&engine);
    return -1;
  }

  ma_sound ma_track;
  for (int i = 0; i < curr_playlist->tracks_len; i++) {
    char *track = curr_playlist->tracks[i];
    result = ma_sound_init_from_file(&engine, track, 0, NULL, NULL, &ma_track);
    if (result == MA_SUCCESS) {
      printf("Starting track: %s\n", track);
      
      ma_sound_start(&ma_track);
      printf("[n] next  [q] quit \n");
      bool quit = false, next = false;
      while (ma_sound_is_playing(&ma_track) && !next && !quit) {
	int c = fgetc_unlocked(stdin);
	if (c == 'n') {
	  next = true;
	} else if (c == 'q') {
	  quit = true;
	}
      }
      
      ma_sound_uninit(&ma_track);
      
      if (quit) {
	break;
      }
    }
  }

  printf("Finished playlist\n");

  ma_sound_uninint(&ma_track);
  ma_engine_uninit(&engine);
  
  free_playlists(playlists, len_playlists);
  
  return 0;
}
