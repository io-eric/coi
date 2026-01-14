<script setup>
import { ref, nextTick } from 'vue'

const adjectives = ['pretty', 'large', 'big', 'small', 'tall', 'short', 'long', 'handsome', 'plain', 'quaint', 'clean', 'elegant', 'easy', 'angry', 'crazy', 'helpful', 'mushy', 'odd', 'unsightly', 'adorable', 'important', 'inexpensive', 'cheap', 'expensive', 'fancy']
const colours = ['red', 'yellow', 'blue', 'green', 'pink', 'brown', 'purple', 'brown', 'white', 'black', 'orange']
const nouns = ['table', 'chair', 'house', 'bbq', 'desk', 'car', 'pony', 'cookie', 'sandwich', 'burger', 'pizza', 'mouse', 'keyboard']

let nextId = 1
let startTime = 0

function buildData(count) {
  const data = []
  for (let i = 0; i < count; i++) {
    data.push({
      id: nextId++,
      label: `${adjectives[Math.floor(Math.random() * adjectives.length)]} ${colours[Math.floor(Math.random() * colours.length)]} ${nouns[Math.floor(Math.random() * nouns.length)]}`
    })
  }
  return data
}

const rows = ref([])
const selected = ref(null)
const result = ref('')

async function measureEnd(operation) {
  await nextTick()
  requestAnimationFrame(() => {
    requestAnimationFrame(() => {
      const duration = performance.now() - startTime
      const msg = `${operation}: ${duration.toFixed(2)}ms`
      result.value = msg
      console.log(msg)
      // Expose result for automated testing
      window.__benchmarkResult = { operation, duration }
    })
  })
}

function create1000() {
  startTime = performance.now()
  rows.value = buildData(1000)
  measureEnd('Create 1000 rows')
}

function updateRows() {
  startTime = performance.now()
  rows.value = rows.value.map((row) => 
    ({ ...row, label: row.label + ' !!!' })
  )
  measureEnd('Update 1,000 rows')
}

function clear() {
  startTime = performance.now()
  rows.value = []
  measureEnd('Clear all rows')
}

function swapRows() {
  if (rows.value.length < 999) return
  startTime = performance.now()
  const next = [...rows.value]
  const temp = next[1]
  next[1] = next[998]
  next[998] = temp
  rows.value = next
  measureEnd('Swap rows')
}

function selectRow(id) {
  startTime = performance.now()
  selected.value = id
  measureEnd('Select row')
}

function removeRow(id) {
  startTime = performance.now()
  rows.value = rows.value.filter(row => row.id !== id)
  measureEnd('Remove row')
}
</script>

<template>
  <div class="container">
    <h1>Vue Rows Benchmark</h1>
    <div class="controls">
      <button id="create1000" @click="create1000">Create 1,000 rows</button>
      <button id="update" @click="updateRows">Update 1,000 rows</button>
      <button id="swap" @click="swapRows">Swap rows</button>
      <button id="clear" @click="clear">Clear</button>
    </div>
    <div v-if="result" class="results" id="result">{{ result }}</div>
    <ul class="row-list">
      <li
        v-for="row in rows"
        :key="row.id"
        :class="['row', { selected: selected === row.id }]"
      >
        <span class="row-id">{{ row.id }}</span>
        <span class="row-label" @click="selectRow(row.id)">{{ row.label }}</span>
        <span class="row-actions">
          <button @click="removeRow(row.id)">×</button>
        </span>
      </li>
    </ul>
  </div>
</template>
