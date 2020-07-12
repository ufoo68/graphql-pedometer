const { MQTTPubSub } = require('graphql-mqtt-subscriptions')
const { connect } = require('mqtt')
const dayjs = require('dayjs')

const client = connect('mqtt://mqtt.eclipse.org', {
  reconnectPeriod: 1000
})

const pubsub = new MQTTPubSub({
  client
})

const resolvers = {
  Query: {
    sensors: () => {
      return [{ id: "Sensor1" }, { id: "Sensor2" }];
    }
  },
  Subscription: {
    subscribe2sensor: {
      resolve: (payload) => ({
        steps: payload.data.steps,
        time: dayjs(payload.data.time).toISOString()
      }),
      subscribe: (_, args) => pubsub.asyncIterator([args.topic])
    },
  }
}

module.exports = { resolvers }