<script setup>
import { ref, watch, onMounted } from 'vue'
import axios from 'axios'

// data
const data = ref({
  ip_address: "",
  target: "",
  cores: 0,
  chip: "",
  revision: "",
  flash: 0,
  memo: ""
})
// WebSocket
var ws = null
const isWSConnected = ref(false)

onMounted(() => {
  // データ取得
  axios
    .get(encodeURI("./API/get_data"))
    .then(response => {
      if (response.headers["content-type"] == "application/json") {
        data.value = response.data
      }
    })
    .catch(error => {
      log.console("./API/get_data request error")
    })
  // WebSocket接続
  const url = "ws://" + window.location.host + "/ws"
  ws = new WebSocket(url)
  ws.onopen = (event) => {
    console.log("WS : サーバーConnect")
    isWSConnected.value = true
  }
  ws.onclose = (event) => {
    console.log("WS : サーバーDisconnect")
    isWSConnected.value = false
  }
  ws.onmessage = (event) => {
    console.log("WS : サーバーから受信 : " + event.data)
  }
  ws.onerror = (event) => {
    console.error("WS : エラー", event)
  }
})

function save() {
  axios
    .post(encodeURI("./API/set_data"), { memo: data.value.memo }, { headers: { 'Content-Type': 'application/json'}})
    .then(response => {
      axios
        .post(encodeURI("./API/save"), "", { headers: { 'Content-Type': 'application/json'}})
    })
}

</script>

<template>
  <main>
    <div style="position: absolute; top: 10px; left: 10px; right: 0;">
      IP Address : {{ data.ip_address }}<br/>
      Target : {{ data.target }}<br/>
      Cores : {{ data.cores }}<br/>
      Chip : {{ data.chip }}<br/>
      Revision : {{ data.revision }}<br/>
      Flash : {{ data.flash }}<br/>
      <div class="container text-center">
        <div class="row">
          <div class="col">
            <h1>メモ</h1>
          </div>
        </div>
        <div class="row">
          <div class="col">
            <hr>
          </div>
        </div>
        <div class="row" style="margin-top: 5px;">
          <div class="col">
            <input v-model="data.memo">
          </div>
        </div>
        <dev class="row">
          <div class="col">
            <button class="btn btn-success" style="margin-top: 5em;" @click="save">Save</button>
          </div>
        </dev>
      </div>
    </div>
  </main>
</template>

<style scoped>
h1,hr {
  margin: 1px;
  padding: 1px;
}

.zeromp {
  margin: 0;
  padding: 0;
}

button {
  padding: 1em 2em 1em;
}

div {
  color: white;
}
</style>
