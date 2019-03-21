#include "audio.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#include "fs.h"
#include "textures.h"

static drflac *flac;
static drflac_uint64 frames_read = 0;

static void FLAC_SplitVorbisComments(char *comment, char *tag, char *value){
	char *result = NULL;
	result = strtok(comment, "=");
	int count = 0;

	while((result != NULL) && (count < 2)) {
		if (strlen(result) > 0) {
			switch (count) {
				case 0:
					strncpy(tag, result, 30);
					tag[30] = '\0';
					break;
				case 1:
					strncpy(value, result, 255);
					value[255] = '\0';
					break;
			}

			count++;
		}
		result = strtok(NULL, "=");
	}
}

static void FLAC_MetaCallback(void *pUserData, drflac_metadata *pMetadata) {
	char tag[31];
	char value[256];
	
	if (pMetadata->type == DRFLAC_METADATA_BLOCK_TYPE_VORBIS_COMMENT) {
		drflac_vorbis_comment_iterator iterator;
		drflac_uint32 comment_length;
		const char *comment_str;

		drflac_init_vorbis_comment_iterator(&iterator, pMetadata->data.vorbis_comment.commentCount, pMetadata->data.vorbis_comment.pComments);

		while((comment_str = drflac_next_vorbis_comment(&iterator, &comment_length)) != NULL) {
			FLAC_SplitVorbisComments((char *)comment_str, tag, value);
			if (!strcasecmp(tag, "TITLE")) {
				strcpy(title, value);
			}
			if (!strcasecmp(tag, "ARTIST")) {
				strcpy(artist, value);
			}
		}
	}
	if (pMetadata->type == DRFLAC_METADATA_BLOCK_TYPE_PICTURE) {
		if (pMetadata->data.picture.type == DRFLAC_PICTURE_TYPE_COVER_FRONT) {
			cover_image = g2dTexLoadMemory((drflac_uint8 *)pMetadata->data.picture.pPictureData, pMetadata->data.picture.pictureDataSize, G2D_SWIZZLE);
		}
	}
}

int FLAC_Init(const char *path) {
	flac = drflac_open_file_with_metadata(path, FLAC_MetaCallback, NULL);
	if (flac == NULL)
		return -1;

	return 0;
}

void FLAC_Decode(void *buf, unsigned int length, void *userdata) {
	frames_read += drflac_read_pcm_frames_s16(flac, (drflac_uint64)length, (drflac_int16 *)buf);
	
	if (frames_read == flac->totalPCMFrameCount)
		playing = false;
}

u64 FLAC_GetPosition(void) {
	return frames_read;
}

u64 FLAC_GetLength(void) {
	return flac->totalPCMFrameCount;
}

u64 FLAC_GetPositionSeconds(const char *path) {
	return (frames_read / flac->sampleRate);
}

u64 FLAC_GetLengthSeconds(const char *path) {
	return (flac->totalPCMFrameCount / flac->sampleRate);
}

void FLAC_Term(void) {
	frames_read = 0;
	drflac_close(flac);
}
