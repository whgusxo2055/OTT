const video = document.querySelector('video');
const form = document.querySelector('form');
const contentIdInput = form.elements.contentId;
const button = form.querySelector('button');
const status = document.querySelector('[role="status"]');
let hls;

function failed(message) {
  status.textContent = message;
  button.disabled = false;
}

video.addEventListener('loadedmetadata', () => {
  status.textContent = '영상을 불러왔습니다. 플레이어에서 재생하세요.';
  button.disabled = false;
});

video.addEventListener('error', () => failed('영상을 재생할 수 없습니다.'));

async function loadContent(contentId) {
  if (!contentId) {
    failed('콘텐츠 ID를 입력하세요.');
    return;
  }

  button.disabled = true;
  status.textContent = '영상을 불러오는 중입니다.';
  hls?.destroy();
  hls = undefined;
  video.removeAttribute('src');
  video.load();

  try {
    const response = await fetch(`/api/v1/contents/${encodeURIComponent(contentId)}/playback`);
    const data = await response.json();
    if (!response.ok) {
      throw new Error(data.error?.message ?? '재생 정보를 가져오지 못했습니다.');
    }

    if (video.canPlayType('application/vnd.apple.mpegurl')) {
      video.src = data.manifestUrl;
    } else if (window.Hls?.isSupported()) {
      hls = new Hls();
      hls.on(Hls.Events.ERROR, (_, data) => {
        if (data.fatal) {
          failed('재생 목록 또는 영상 조각을 읽을 수 없습니다.');
          hls.destroy();
        }
      });
      hls.loadSource(data.manifestUrl);
      hls.attachMedia(video);
    } else {
      failed('이 브라우저에서는 HLS를 재생할 수 없습니다.');
    }
  } catch (error) {
    failed(error.message);
  }
}

form.addEventListener('submit', event => {
  event.preventDefault();
  loadContent(contentIdInput.value.trim());
});

const linkedContentId = new URLSearchParams(location.search).get('contentId')?.trim();
if (linkedContentId) {
  contentIdInput.value = linkedContentId;
  loadContent(linkedContentId);
}
