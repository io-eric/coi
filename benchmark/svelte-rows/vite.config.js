import { defineConfig } from 'vite'
import { svelte } from '@sveltejs/vite-plugin-svelte'

export default defineConfig({
  plugins: [svelte()],
  build: {
    minify: 'terser',
    terserOptions: {
      compress: { drop_console: true }
    }
  }
})
