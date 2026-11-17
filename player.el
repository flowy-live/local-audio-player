;; -*- lexical-binding: t; -*-
(defvar player-program "/home/talksik/programming/audio-playground/play")

(defvar player-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map (kbd "n") #'player-next)
    (define-key map (kbd "q") #'player-quit)
    map))

(define-derived-mode player-mode special-mode "Player"
  "Control the audio player with n (next) and q (quit).")
(defun player-start ()
  (interactive)
  (let* ((buffer (get-buffer-create "*My Player*"))
         (existing (get-buffer-process buffer))
         (playlist (read-string "Playlist name: ")))
    (when (and existing (process-live-p existing))
      (user-error "Player is already running"))

    (with-current-buffer buffer
      (player-mode)
      (let ((inhibit-read-only t))
        (erase-buffer)
        (insert "Audio player controls:\n"
                "  n  Next track\n"
                "  q  Quit player\n\n"
                "Player output:\n\n")))

    (let ((process
           (make-process
            :name "player"
            :buffer buffer
            :command (list player-program
                           "/home/talksik/programming/audio-playground/playlists")
            :connection-type 'pipe
            :noquery t)))
      (process-send-string process (concat playlist "\n")))

    (pop-to-buffer buffer)))

(defun player-next ()
  (interactive)
  (player-send "n\n"))

(defun player-quit ()
  (interactive)
  (interactive)
  (player-send "q\n"))

(defun player-send (text)
  (let ((process (get-buffer-process (current-buffer))))
    (unless (and process (process-live-p process))
      (user-error "Player is not running"))
    (process-send-string process text)))

(provide 'player)
